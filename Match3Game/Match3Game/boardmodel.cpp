#include "boardmodel.h"

#include <QSet>
#include <QTimer>

#include <algorithm>
#include <array>
#include <utility>

namespace
{
using ItemType = BoardModel::ItemType;

constexpr int kBoardRows = 8;
constexpr int kBoardColumns = 8;

constexpr std::array<QPoint, 13> kInitialBoxPositions = {
    QPoint(0, 4), QPoint(7, 4),
    QPoint(0, 5), QPoint(6, 5), QPoint(7, 5),
    QPoint(0, 6), QPoint(6, 6), QPoint(7, 6),
    QPoint(0, 7), QPoint(1, 7), QPoint(5, 7), QPoint(6, 7), QPoint(7, 7)
};

QVector<int> uniqueSorted(const QVector<int> &values)
{
    QSet<int> deduped(values.begin(), values.end());
    QVector<int> result = deduped.values().toVector();
    std::sort(result.begin(), result.end());
    return result;
}

bool containsValue(const QVector<int> &values, int value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}
}

BoardModel::BoardModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_rng(0xC0FFEEu)
{
    initializeBoard();
}

int BoardModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_cells.size();
}

QVariant BoardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_cells.size()) {
        return QVariant();
    }

    const int cellIndex = index.row();
    const QPoint position = fromCellIndex(cellIndex);
    const Cell &cell = m_cells[cellIndex];

    switch (role) {
    case CellRowRole:
        return position.y();
    case CellColumnRole:
        return position.x();
    case CellTypeRole:
        return static_cast<int>(cell.type);
    case CellColorRole:
        return cell.colorId;
    case CellTypeNameRole:
        return typeName(cell.type);
    case SelectedRole:
        return cellIndex == m_selectedIndex;
    case MovableRole:
        return isMovable(cell);
    case LabelRole:
        return cellLabel(cell);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> BoardModel::roleNames() const
{
    return {
        { CellRowRole, "cellRow" },
        { CellColumnRole, "cellColumn" },
        { CellTypeRole, "cellType" },
        { CellColorRole, "cellColor" },
        { CellTypeNameRole, "cellTypeName" },
        { SelectedRole, "selected" },
        { MovableRole, "movable" },
        { LabelRole, "label" }
    };
}

int BoardModel::rows() const
{
    return kRows;
}

int BoardModel::columns() const
{
    return kColumns;
}

bool BoardModel::inputLocked() const
{
    return m_inputLocked;
}

bool BoardModel::gameWon() const
{
    return m_gameWon;
}

int BoardModel::remainingBoxes() const
{
    return m_remainingBoxes;
}

int BoardModel::targetBoxes() const
{
    return m_targetBoxes;
}

quint32 BoardModel::seed() const
{
    return m_seed;
}

QString BoardModel::statusText() const
{
    return m_statusText;
}

void BoardModel::clickCell(int row, int column)
{
    if (m_inputLocked || !isInBounds(row, column)) {
        return;
    }

    const int clickedIndex = toCellIndex(row, column);
    const Cell &clickedCell = m_cells[clickedIndex];

    if (!isMovable(clickedCell)) {
        if (isBox(clickedCell)) {
            setStatusText(QStringLiteral("Boxes are static targets and cannot be swapped."));
        } else {
            setStatusText(QStringLiteral("Empty cells cannot be selected."));
        }
        return;
    }

    if (m_selectedIndex < 0) {
        setSelectedIndex(clickedIndex);
        setStatusText(QStringLiteral("Select one adjacent cell to swap."));
        return;
    }

    if (m_selectedIndex == clickedIndex) {
        setSelectedIndex(-1);
        setStatusText(QStringLiteral("Selection cleared."));
        return;
    }

    if (!isAdjacent(m_selectedIndex, clickedIndex)) {
        setSelectedIndex(clickedIndex);
        setStatusText(QStringLiteral("Only adjacent cells can swap. Selection moved."));
        return;
    }

    if (!canSwap(m_selectedIndex, clickedIndex)) {
        setSelectedIndex(-1);
        setStatusText(QStringLiteral("That swap is not allowed."));
        return;
    }

    startSwap(m_selectedIndex, clickedIndex);
}

void BoardModel::resetBoard()
{
    beginResetModel();
    initializeBoard();
    endResetModel();
    emit inputLockedChanged();
    emit gameWonChanged();
    emit remainingBoxesChanged();
    emit seedChanged();
    emit statusTextChanged();
}

void BoardModel::nextSeed()
{
    ++m_seed;
    resetBoard();
}

void BoardModel::initializeBoard()
{
    m_cells.resize(kRows * kColumns);
    std::fill(m_cells.begin(), m_cells.end(), Cell {});

    const auto isInitialBoxPosition = [](int row, int column) {
        for (const QPoint &position : kInitialBoxPositions) {
            if (position.y() == row && position.x() == column) {
                return true;
            }
        }
        return false;
    };

    m_rng.seed(m_seed);

    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            Cell &cell = cellAt(row, column);
            if (isInitialBoxPosition(row, column)) {
                cell = { ItemType::Box, -1 };
                continue;
            }

            int colorId = randomColorId();
            int guard = 0;
            while (createsImmediateMatch(row, column, colorId) && guard < 20) {
                colorId = randomColorId();
                ++guard;
            }

            cell = { ItemType::Normal, colorId };
        }
    }

    m_remainingBoxes = 0;
    for (const Cell &cell : std::as_const(m_cells)) {
        if (cell.type == ItemType::Box) {
            ++m_remainingBoxes;
        }
    }

    m_targetBoxes = m_remainingBoxes;
    m_selectedIndex = -1;
    m_pendingSwap.clear();
    m_inputLocked = false;
    m_gameWon = false;
    m_state = State::Idle;
    m_chainCount = 0;
    m_statusText = QStringLiteral("参考视频样式已对齐：固定箱子布局，初始只生成普通元素，同一 seed 可复现。");
}

void BoardModel::setSelectedIndex(int index)
{
    if (m_selectedIndex == index) {
        return;
    }

    const int previousIndex = m_selectedIndex;
    m_selectedIndex = index;

    if (previousIndex >= 0) {
        const QModelIndex previousModelIndex = QAbstractListModel::index(previousIndex, 0);
        emit dataChanged(previousModelIndex, previousModelIndex, { SelectedRole });
    }

    if (m_selectedIndex >= 0) {
        const QModelIndex currentModelIndex = QAbstractListModel::index(m_selectedIndex, 0);
        emit dataChanged(currentModelIndex, currentModelIndex, { SelectedRole });
    }
}

void BoardModel::setInputLocked(bool locked)
{
    if (m_inputLocked == locked) {
        return;
    }

    m_inputLocked = locked;
    emit inputLockedChanged();
}

void BoardModel::setStatusText(const QString &text)
{
    if (m_statusText == text) {
        return;
    }

    m_statusText = text;
    emit statusTextChanged();
}

void BoardModel::notifyBoardChanged()
{
    if (m_cells.isEmpty()) {
        return;
    }

    const QModelIndex first = QAbstractListModel::index(0, 0);
    const QModelIndex last = QAbstractListModel::index(m_cells.size() - 1, 0);
    emit dataChanged(first, last);
}

bool BoardModel::isInBounds(int row, int column) const
{
    return row >= 0 && row < kRows && column >= 0 && column < kColumns;
}

int BoardModel::toCellIndex(int row, int column) const
{
    return row * kColumns + column;
}

QPoint BoardModel::fromCellIndex(int index) const
{
    return QPoint(index % kColumns, index / kColumns);
}

const BoardModel::Cell &BoardModel::cellAt(int row, int column) const
{
    return m_cells[toCellIndex(row, column)];
}

BoardModel::Cell &BoardModel::cellAt(int row, int column)
{
    return m_cells[toCellIndex(row, column)];
}

bool BoardModel::isEmptyCell(const Cell &cell) const
{
    return cell.type == ItemType::Empty;
}

bool BoardModel::isBox(const Cell &cell) const
{
    return cell.type == ItemType::Box;
}

bool BoardModel::isRocket(const Cell &cell) const
{
    return cell.type == ItemType::RocketHorizontal || cell.type == ItemType::RocketVertical;
}

bool BoardModel::isSpecial(const Cell &cell) const
{
    return isRocket(cell) || cell.type == ItemType::Bomb || cell.type == ItemType::Propeller;
}

bool BoardModel::isMovable(const Cell &cell) const
{
    return cell.type == ItemType::Normal || isSpecial(cell);
}

bool BoardModel::isMatchable(const Cell &cell) const
{
    return cell.type == ItemType::Normal && cell.colorId >= 0;
}

bool BoardModel::isAdjacent(int first, int second) const
{
    const QPoint firstPoint = fromCellIndex(first);
    const QPoint secondPoint = fromCellIndex(second);
    return std::abs(firstPoint.x() - secondPoint.x()) + std::abs(firstPoint.y() - secondPoint.y()) == 1;
}

bool BoardModel::canSwap(int first, int second) const
{
    return isAdjacent(first, second) && isMovable(m_cells[first]) && isMovable(m_cells[second]);
}

void BoardModel::startSwap(int first, int second)
{
    setSelectedIndex(-1);
    setInputLocked(true);
    m_chainCount = 0;
    setStatusText(QStringLiteral("Swapping..."));

    std::swap(m_cells[first], m_cells[second]);
    m_pendingSwap = { first, second };
    m_state = State::Swapping;
    notifyBoardChanged();

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        finishSwapEvaluation();
    });
}

void BoardModel::finishSwapEvaluation()
{
    if (m_pendingSwap.isValid()) {
        const Cell &firstCell = m_cells[m_pendingSwap.first];
        const Cell &secondCell = m_cells[m_pendingSwap.second];
        if (isSpecial(firstCell) || isSpecial(secondCell)) {
            activateSpecialSwap();
            return;
        }
    }

    const MatchInfo matchInfo = analyzeMatches();
    if (matchInfo.isEmpty()) {
        revertSwap();
        return;
    }

    resolveMatchInfo(matchInfo);
}

void BoardModel::revertSwap()
{
    if (!m_pendingSwap.isValid()) {
        finishTurn();
        return;
    }

    m_state = State::Reverting;
    std::swap(m_cells[m_pendingSwap.first], m_cells[m_pendingSwap.second]);
    notifyBoardChanged();
    setStatusText(QStringLiteral("No valid match or tool trigger formed. Swap reverted."));

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        finishTurn();
    });
}

void BoardModel::processCascade()
{
    const MatchInfo matchInfo = analyzeMatches();
    if (matchInfo.isEmpty()) {
        finishTurn();
        return;
    }

    resolveMatchInfo(matchInfo);
}

void BoardModel::finishTurn()
{
    m_pendingSwap.clear();
    m_state = State::Idle;

    if (m_remainingBoxes == 0) {
        markVictory();
        return;
    }

    setInputLocked(false);

    if (m_chainCount > 0) {
        setStatusText(QStringLiteral("Turn complete. Resolve rounds: %1.").arg(m_chainCount));
    } else {
        setStatusText(QStringLiteral("Click two adjacent cells to swap."));
    }
}

void BoardModel::markVictory()
{
    if (!m_gameWon) {
        m_gameWon = true;
        emit gameWonChanged();
    }

    setInputLocked(true);
    setStatusText(QStringLiteral("Great! All boxes cleared. Reset with the same seed or advance to a new seed."));
}

BoardModel::MatchInfo BoardModel::analyzeMatches() const
{
    MatchInfo info;

    const QVector<LineGroup> horizontalGroups = findLineGroups(true);
    const QVector<LineGroup> verticalGroups = findLineGroups(false);
    const QVector<QVector<int>> squareGroups = findSquareGroups();

    QSet<int> matchedSet;
    for (const LineGroup &group : horizontalGroups) {
        for (const int index : group.cells) {
            matchedSet.insert(index);
        }
    }
    for (const LineGroup &group : verticalGroups) {
        for (const int index : group.cells) {
            matchedSet.insert(index);
        }
    }
    for (const QVector<int> &group : squareGroups) {
        for (const int index : group) {
            matchedSet.insert(index);
        }
    }

    info.matchedCells = matchedSet.values().toVector();
    std::sort(info.matchedCells.begin(), info.matchedCells.end());

    QVector<SpawnRequest> spawns;
    QVector<int> reservedCells;
    const QVector<int> preferredOrder = { m_pendingSwap.second, m_pendingSwap.first };

    for (const LineGroup &horizontal : horizontalGroups) {
        for (const LineGroup &vertical : verticalGroups) {
            QVector<int> intersection;
            for (const int horizontalIndex : horizontal.cells) {
                if (containsValue(vertical.cells, horizontalIndex)) {
                    intersection.push_back(horizontalIndex);
                }
            }

            if (intersection.isEmpty()) {
                continue;
            }

            QSet<int> unionSet(horizontal.cells.begin(), horizontal.cells.end());
            for (const int index : vertical.cells) {
                unionSet.insert(index);
            }
            if (unionSet.size() < 5) {
                continue;
            }

            const int spawnIndex = chooseSpawnIndex(intersection, preferredOrder, {});
            if (spawnIndex >= 0) {
                addSpawnRequest(&spawns, { spawnIndex, ItemType::Bomb, 3 });
            }

            reservedCells += unionSet.values().toVector();
        }
    }

    for (const LineGroup &group : horizontalGroups) {
        if (group.cells.size() < 4) {
            continue;
        }

        bool overlapsReserved = false;
        for (const int index : group.cells) {
            if (containsValue(reservedCells, index)) {
                overlapsReserved = true;
                break;
            }
        }

        if (overlapsReserved) {
            continue;
        }

        const int spawnIndex = chooseSpawnIndex(group.cells, preferredOrder, reservedCells);
        if (spawnIndex >= 0) {
            addSpawnRequest(&spawns, { spawnIndex, ItemType::RocketVertical, 2 });
            reservedCells += group.cells;
        }
    }

    for (const LineGroup &group : verticalGroups) {
        if (group.cells.size() < 4) {
            continue;
        }

        bool overlapsReserved = false;
        for (const int index : group.cells) {
            if (containsValue(reservedCells, index)) {
                overlapsReserved = true;
                break;
            }
        }

        if (overlapsReserved) {
            continue;
        }

        const int spawnIndex = chooseSpawnIndex(group.cells, preferredOrder, reservedCells);
        if (spawnIndex >= 0) {
            addSpawnRequest(&spawns, { spawnIndex, ItemType::RocketHorizontal, 2 });
            reservedCells += group.cells;
        }
    }

    for (const QVector<int> &square : squareGroups) {
        bool overlapsReserved = false;
        for (const int index : square) {
            if (containsValue(reservedCells, index)) {
                overlapsReserved = true;
                break;
            }
        }

        if (overlapsReserved) {
            continue;
        }

        const int spawnIndex = chooseSpawnIndex(square, preferredOrder, reservedCells);
        if (spawnIndex >= 0) {
            addSpawnRequest(&spawns, { spawnIndex, ItemType::Propeller, 1 });
            reservedCells += square;
        }
    }

    info.spawns = spawns;
    return info;
}

QVector<BoardModel::LineGroup> BoardModel::findLineGroups(bool horizontal) const
{
    QVector<LineGroup> groups;

    const int outerLimit = horizontal ? kRows : kColumns;
    const int innerLimit = horizontal ? kColumns : kRows;

    for (int outer = 0; outer < outerLimit; ++outer) {
        int inner = 0;
        while (inner < innerLimit) {
            const int row = horizontal ? outer : inner;
            const int column = horizontal ? inner : outer;
            const Cell &startCell = cellAt(row, column);

            if (!isMatchable(startCell)) {
                ++inner;
                continue;
            }

            int end = inner + 1;
            while (end < innerLimit) {
                const int candidateRow = horizontal ? outer : end;
                const int candidateColumn = horizontal ? end : outer;
                const Cell &candidate = cellAt(candidateRow, candidateColumn);
                if (!isMatchable(candidate) || candidate.colorId != startCell.colorId) {
                    break;
                }
                ++end;
            }

            if (end - inner >= 3) {
                LineGroup group;
                group.horizontal = horizontal;
                group.colorId = startCell.colorId;

                for (int current = inner; current < end; ++current) {
                    const int groupRow = horizontal ? outer : current;
                    const int groupColumn = horizontal ? current : outer;
                    group.cells.push_back(toCellIndex(groupRow, groupColumn));
                }

                groups.push_back(group);
            }

            inner = end;
        }
    }

    return groups;
}

QVector<QVector<int>> BoardModel::findSquareGroups() const
{
    QVector<QVector<int>> groups;

    for (int row = 0; row < kRows - 1; ++row) {
        for (int column = 0; column < kColumns - 1; ++column) {
            const Cell &topLeft = cellAt(row, column);
            const Cell &topRight = cellAt(row, column + 1);
            const Cell &bottomLeft = cellAt(row + 1, column);
            const Cell &bottomRight = cellAt(row + 1, column + 1);

            if (!isMatchable(topLeft)) {
                continue;
            }

            const int colorId = topLeft.colorId;
            if (!isMatchable(topRight) || topRight.colorId != colorId
                || !isMatchable(bottomLeft) || bottomLeft.colorId != colorId
                || !isMatchable(bottomRight) || bottomRight.colorId != colorId) {
                continue;
            }

            groups.push_back({
                toCellIndex(row, column),
                toCellIndex(row, column + 1),
                toCellIndex(row + 1, column),
                toCellIndex(row + 1, column + 1)
            });
        }
    }

    return groups;
}

int BoardModel::chooseSpawnIndex(const QVector<int> &candidates, const QVector<int> &preferredOrder, const QVector<int> &reservedCells) const
{
    for (const int preferredIndex : preferredOrder) {
        if (preferredIndex >= 0 && containsValue(candidates, preferredIndex) && !containsValue(reservedCells, preferredIndex)) {
            return preferredIndex;
        }
    }

    for (const int candidateIndex : candidates) {
        if (!containsValue(reservedCells, candidateIndex)) {
            return candidateIndex;
        }
    }

    return candidates.isEmpty() ? -1 : candidates.front();
}

void BoardModel::addSpawnRequest(QVector<SpawnRequest> *spawns, const SpawnRequest &request) const
{
    if (request.index < 0) {
        return;
    }

    for (SpawnRequest &existing : *spawns) {
        if (existing.index != request.index) {
            continue;
        }

        if (request.priority >= existing.priority) {
            existing = request;
        }
        return;
    }

    spawns->push_back(request);
}

void BoardModel::resolveMatchInfo(const MatchInfo &matchInfo)
{
    ++m_chainCount;
    clearMatchedCells(matchInfo);

    m_state = State::Clearing;
    if (matchInfo.spawns.isEmpty()) {
        setStatusText(QStringLiteral("Resolve round %1: clear matches, then fall and refill.").arg(m_chainCount));
    } else {
        setStatusText(QStringLiteral("Resolve round %1: clear matches and generate %2 tool(s).").arg(m_chainCount).arg(matchInfo.spawns.size()));
    }

    notifyBoardChanged();

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        m_state = State::Falling;
        applyGravity();
        refillEmptyCells();
        notifyBoardChanged();

        setStatusText(QStringLiteral("Board settled. Checking for another cascade..."));

        QTimer::singleShot(kAnimationDelayMs, this, [this]() {
            processCascade();
        });
    });
}

void BoardModel::clearMatchedCells(const MatchInfo &matchInfo)
{
    QVector<int> hitCells;
    QVector<int> spawnIndices;
    for (const SpawnRequest &spawn : matchInfo.spawns) {
        spawnIndices.push_back(spawn.index);
    }

    for (const int index : matchInfo.matchedCells) {
        if (index < 0 || index >= m_cells.size()) {
            continue;
        }

        if (containsValue(spawnIndices, index)) {
            hitCells.push_back(index);
            continue;
        }

        if (clearMovableAt(index)) {
            hitCells.push_back(index);
        }
    }

    clearAdjacentBoxes(hitCells);
    placeGeneratedSpecials(matchInfo.spawns);
}

void BoardModel::placeGeneratedSpecials(const QVector<SpawnRequest> &spawns)
{
    for (const SpawnRequest &spawn : spawns) {
        if (spawn.index < 0 || spawn.index >= m_cells.size()) {
            continue;
        }

        m_cells[spawn.index] = { spawn.type, -1 };
    }
}

void BoardModel::activateSpecialSwap()
{
    if (!m_pendingSwap.isValid()) {
        finishTurn();
        return;
    }

    const int firstIndex = m_pendingSwap.first;
    const int secondIndex = m_pendingSwap.second;
    const Cell &firstCell = m_cells[firstIndex];
    const Cell &secondCell = m_cells[secondIndex];

    const bool firstIsPropeller = firstCell.type == ItemType::Propeller;
    const bool secondIsPropeller = secondCell.type == ItemType::Propeller;
    const bool firstIsRocket = isRocket(firstCell);
    const bool secondIsRocket = isRocket(secondCell);

    if ((firstIsPropeller && secondIsRocket) || (secondIsPropeller && firstIsRocket)) {
        const int propellerIndex = firstIsPropeller ? firstIndex : secondIndex;
        const int rocketIndex = firstIsRocket ? firstIndex : secondIndex;

        EffectResult effect;
        effect.hitCells.push_back(propellerIndex);
        effect.hitCells.push_back(rocketIndex);

        const int oppositeRocketIndex = findOppositeRocketTarget(rocketIndex);
        if (oppositeRocketIndex >= 0) {
            const QPoint oppositePoint = fromCellIndex(oppositeRocketIndex);
            addLineEffect(oppositePoint.y(), oppositePoint.x(), true, &effect.hitCells, &effect.adjacentBoxHits);
            addLineEffect(oppositePoint.y(), oppositePoint.x(), false, &effect.hitCells, &effect.adjacentBoxHits);
            effect.status = QStringLiteral("Rocket + propeller combo: cross clear fires from the opposite rocket.");
        } else {
            addRocketEffect(rocketIndex, &effect.hitCells, &effect.adjacentBoxHits);
            effect.status = QStringLiteral("Rocket + propeller combo fell back to the swapped rocket effect.");
        }

        resolveEffectResult(effect);
        return;
    }

    QVector<int> specialIndices;
    if (isSpecial(firstCell)) {
        specialIndices.push_back(firstIndex);
    }
    if (isSpecial(secondCell)) {
        specialIndices.push_back(secondIndex);
    }

    activateSpecialItems(specialIndices, true, QStringLiteral("Special item activated by swap."));
}

void BoardModel::activateSpecialItems(const QVector<int> &specialIndices, bool allowPropellerTarget, const QString &statusText)
{
    EffectResult effect;
    const QVector<int> uniqueIndices = uniqueSorted(specialIndices);

    for (const int index : uniqueIndices) {
        if (index < 0 || index >= m_cells.size()) {
            continue;
        }

        const Cell &cell = m_cells[index];
        if (isRocket(cell)) {
            addRocketEffect(index, &effect.hitCells, &effect.adjacentBoxHits);
        } else if (cell.type == ItemType::Bomb) {
            addBombEffect(index, &effect.hitCells, &effect.directBoxHits);
        } else if (cell.type == ItemType::Propeller) {
            addPropellerEffect(index, allowPropellerTarget, &effect.hitCells, &effect.directBoxHits);
        }
    }

    effect.status = statusText;
    resolveEffectResult(effect);
}

void BoardModel::resolveEffectResult(const EffectResult &effect)
{
    ++m_chainCount;

    const QVector<int> hitCells = uniqueSorted(effect.hitCells);
    const QVector<int> adjacentBoxes = uniqueSorted(effect.adjacentBoxHits);
    const QVector<int> directBoxes = uniqueSorted(effect.directBoxHits);

    for (const int index : hitCells) {
        clearMovableAt(index);
    }

    QSet<int> allBoxes(adjacentBoxes.begin(), adjacentBoxes.end());
    for (const int boxIndex : directBoxes) {
        allBoxes.insert(boxIndex);
    }

    bool removedAnyBox = false;
    for (const int boxIndex : allBoxes) {
        removedAnyBox = clearBoxAt(boxIndex) || removedAnyBox;
    }

    if (removedAnyBox) {
        emit remainingBoxesChanged();
    }

    m_state = State::Clearing;
    setStatusText(effect.status.isEmpty()
        ? QStringLiteral("Special effect resolved.")
        : effect.status);
    notifyBoardChanged();

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        m_state = State::Falling;
        applyGravity();
        refillEmptyCells();
        notifyBoardChanged();

        setStatusText(QStringLiteral("Board settled. Checking for another cascade..."));

        QTimer::singleShot(kAnimationDelayMs, this, [this]() {
            processCascade();
        });
    });
}

void BoardModel::addLineEffect(int row, int column, bool horizontal, QVector<int> *hitCells, QVector<int> *adjacentBoxHits) const
{
    if (horizontal) {
        for (int currentColumn = 0; currentColumn < kColumns; ++currentColumn) {
            const int index = toCellIndex(row, currentColumn);
            hitCells->push_back(index);

            for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
                const int nextColumn = currentColumn + offset.x();
                const int nextRow = row + offset.y();
                if (!isInBounds(nextRow, nextColumn)) {
                    continue;
                }

                const int candidateIndex = toCellIndex(nextRow, nextColumn);
                if (isBox(m_cells[candidateIndex])) {
                    adjacentBoxHits->push_back(candidateIndex);
                }
            }
        }
    } else {
        for (int currentRow = 0; currentRow < kRows; ++currentRow) {
            const int index = toCellIndex(currentRow, column);
            hitCells->push_back(index);

            for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
                const int nextColumn = column + offset.x();
                const int nextRow = currentRow + offset.y();
                if (!isInBounds(nextRow, nextColumn)) {
                    continue;
                }

                const int candidateIndex = toCellIndex(nextRow, nextColumn);
                if (isBox(m_cells[candidateIndex])) {
                    adjacentBoxHits->push_back(candidateIndex);
                }
            }
        }
    }
}

void BoardModel::addRocketEffect(int index, QVector<int> *hitCells, QVector<int> *adjacentBoxHits) const
{
    if (index < 0 || index >= m_cells.size()) {
        return;
    }

    const QPoint position = fromCellIndex(index);
    const Cell &rocketCell = m_cells[index];
    if (rocketCell.type == ItemType::RocketHorizontal) {
        addLineEffect(position.y(), position.x(), true, hitCells, adjacentBoxHits);
    } else if (rocketCell.type == ItemType::RocketVertical) {
        addLineEffect(position.y(), position.x(), false, hitCells, adjacentBoxHits);
    }
}

void BoardModel::addBombEffect(int index, QVector<int> *hitCells, QVector<int> *directBoxHits) const
{
    if (index < 0 || index >= m_cells.size()) {
        return;
    }

    const QPoint center = fromCellIndex(index);
    for (int row = center.y() - 2; row <= center.y() + 2; ++row) {
        for (int column = center.x() - 2; column <= center.x() + 2; ++column) {
            if (!isInBounds(row, column)) {
                continue;
            }

            const int candidateIndex = toCellIndex(row, column);
            hitCells->push_back(candidateIndex);
            if (isBox(m_cells[candidateIndex])) {
                directBoxHits->push_back(candidateIndex);
            }
        }
    }
}

void BoardModel::addPropellerEffect(int index, bool allowBoxTarget, QVector<int> *hitCells, QVector<int> *directBoxHits)
{
    if (index < 0 || index >= m_cells.size()) {
        return;
    }

    hitCells->push_back(index);

    if (!allowBoxTarget) {
        return;
    }

    const int targetBoxIndex = chooseTopLayerBox();
    if (targetBoxIndex >= 0) {
        directBoxHits->push_back(targetBoxIndex);
    }
}

int BoardModel::findOppositeRocketTarget(int rocketIndex) const
{
    if (rocketIndex < 0 || rocketIndex >= m_cells.size()) {
        return -1;
    }

    const ItemType currentType = m_cells[rocketIndex].type;
    const ItemType oppositeType = currentType == ItemType::RocketHorizontal
        ? ItemType::RocketVertical
        : ItemType::RocketHorizontal;

    for (int index = 0; index < m_cells.size(); ++index) {
        if (index == rocketIndex) {
            continue;
        }

        if (m_cells[index].type == oppositeType) {
            return index;
        }
    }

    return -1;
}

int BoardModel::chooseTopLayerBox()
{
    int topRow = kRows;
    QVector<int> candidates;

    for (int index = 0; index < m_cells.size(); ++index) {
        if (!isBox(m_cells[index])) {
            continue;
        }

        const QPoint position = fromCellIndex(index);
        if (position.y() < topRow) {
            topRow = position.y();
            candidates.clear();
            candidates.push_back(index);
        } else if (position.y() == topRow) {
            candidates.push_back(index);
        }
    }

    if (candidates.isEmpty()) {
        return -1;
    }

    return candidates[m_rng.bounded(candidates.size())];
}

void BoardModel::clearAdjacentBoxes(const QVector<int> &hitCells)
{
    QVector<int> boxIndices;

    for (const int hitIndex : uniqueSorted(hitCells)) {
        if (hitIndex < 0 || hitIndex >= m_cells.size()) {
            continue;
        }

        const QPoint position = fromCellIndex(hitIndex);
        for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
            const int nextColumn = position.x() + offset.x();
            const int nextRow = position.y() + offset.y();
            if (!isInBounds(nextRow, nextColumn)) {
                continue;
            }

            const int candidateIndex = toCellIndex(nextRow, nextColumn);
            if (isBox(m_cells[candidateIndex])) {
                boxIndices.push_back(candidateIndex);
            }
        }
    }

    bool removedAny = false;
    for (const int boxIndex : uniqueSorted(boxIndices)) {
        removedAny = clearBoxAt(boxIndex) || removedAny;
    }

    if (removedAny) {
        emit remainingBoxesChanged();
    }
}

bool BoardModel::clearBoxAt(int index)
{
    if (index < 0 || index >= m_cells.size() || !isBox(m_cells[index])) {
        return false;
    }

    m_cells[index] = {};
    m_remainingBoxes = std::max(0, m_remainingBoxes - 1);
    return true;
}

bool BoardModel::clearMovableAt(int index)
{
    if (index < 0 || index >= m_cells.size()) {
        return false;
    }

    Cell &cell = m_cells[index];
    if (cell.type == ItemType::Empty || cell.type == ItemType::Box) {
        return false;
    }

    cell = {};
    return true;
}

bool BoardModel::applyGravity()
{
    bool moved = false;
    while (applyGravityPass()) {
        moved = true;
    }
    return moved;
}

bool BoardModel::applyGravityPass()
{
    bool moved = false;

    for (int row = kRows - 1; row >= 0; --row) {
        for (int column = 0; column < kColumns; ++column) {
            Cell &targetCell = cellAt(row, column);
            if (!isEmptyCell(targetCell)) {
                continue;
            }

            const int aboveRow = row - 1;
            if (aboveRow >= 0) {
                Cell &aboveCell = cellAt(aboveRow, column);
                if (isMovable(aboveCell)) {
                    targetCell = aboveCell;
                    aboveCell = {};
                    moved = true;
                    continue;
                }
            }

            const int diagonalSource = chooseDiagonalSource(row, column);
            if (diagonalSource >= 0) {
                const QPoint sourcePoint = fromCellIndex(diagonalSource);
                targetCell = cellAt(sourcePoint.y(), sourcePoint.x());
                cellAt(sourcePoint.y(), sourcePoint.x()) = {};
                moved = true;
            }
        }
    }

    return moved;
}

int BoardModel::chooseDiagonalSource(int row, int column)
{
    const int sourceRow = row - 1;
    if (sourceRow < 0) {
        return -1;
    }

    QVector<int> candidates;

    if (column > 0 && isMovable(cellAt(sourceRow, column - 1)) && canSlideDiagonally(sourceRow, column - 1)) {
        candidates.push_back(toCellIndex(sourceRow, column - 1));
    }

    if (column + 1 < kColumns && isMovable(cellAt(sourceRow, column + 1)) && canSlideDiagonally(sourceRow, column + 1)) {
        candidates.push_back(toCellIndex(sourceRow, column + 1));
    }

    if (candidates.isEmpty()) {
        return -1;
    }

    if (candidates.size() == 1) {
        return candidates.front();
    }

    return candidates[m_rng.bounded(candidates.size())];
}

bool BoardModel::canSlideDiagonally(int sourceRow, int sourceColumn) const
{
    const int belowRow = sourceRow + 1;
    if (!isInBounds(belowRow, sourceColumn)) {
        return false;
    }

    return !isEmptyCell(cellAt(belowRow, sourceColumn));
}

bool BoardModel::refillEmptyCells()
{
    bool filled = false;

    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            Cell &cell = cellAt(row, column);
            if (!isEmptyCell(cell)) {
                continue;
            }

            cell = randomNormalCell();
            filled = true;
        }
    }

    return filled;
}

int BoardModel::randomColorId()
{
    return m_rng.bounded(kColorCount);
}

BoardModel::Cell BoardModel::randomNormalCell()
{
    return { ItemType::Normal, randomColorId() };
}

bool BoardModel::createsImmediateMatch(int row, int column, int colorId) const
{
    if (column >= 2) {
        const Cell &left = cellAt(row, column - 1);
        const Cell &leftLeft = cellAt(row, column - 2);
        if (isMatchable(left) && isMatchable(leftLeft)
            && left.colorId == colorId && leftLeft.colorId == colorId) {
            return true;
        }
    }

    if (row >= 2) {
        const Cell &up = cellAt(row - 1, column);
        const Cell &upUp = cellAt(row - 2, column);
        if (isMatchable(up) && isMatchable(upUp)
            && up.colorId == colorId && upUp.colorId == colorId) {
            return true;
        }
    }

    return false;
}

QString BoardModel::typeName(ItemType type) const
{
    switch (type) {
    case ItemType::Empty:
        return QStringLiteral("empty");
    case ItemType::Normal:
        return QStringLiteral("normal");
    case ItemType::Box:
        return QStringLiteral("box");
    case ItemType::RocketHorizontal:
        return QStringLiteral("rocketHorizontal");
    case ItemType::RocketVertical:
        return QStringLiteral("rocketVertical");
    case ItemType::Bomb:
        return QStringLiteral("bomb");
    case ItemType::Propeller:
        return QStringLiteral("propeller");
    }

    return QStringLiteral("unknown");
}

QString BoardModel::cellLabel(const Cell &cell) const
{
    switch (cell.type) {
    case ItemType::Empty:
        return QStringLiteral("");
    case ItemType::Normal:
        return QString::number(cell.colorId + 1);
    case ItemType::Box:
        return QStringLiteral("BOX");
    case ItemType::RocketHorizontal:
        return QStringLiteral("ROW");
    case ItemType::RocketVertical:
        return QStringLiteral("COL");
    case ItemType::Bomb:
        return QStringLiteral("BOMB");
    case ItemType::Propeller:
        return QStringLiteral("PROP");
    }

    return QString();
}
