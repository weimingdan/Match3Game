#include "boardmodel.h"

#include <QTimer>

#include <cstdlib>
#include <utility>

BoardModel::BoardModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_statusText = QStringLiteral("Demo ready. Clear all boxes on the 9x8 board.");
}

int BoardModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_board.cellCount();
}

QVariant BoardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_board.cellCount()) {
        return QVariant();
    }

    const int cellIndex = index.row();
    const QPoint position = m_board.fromCellIndex(cellIndex);
    const Match3::Cell &cell = m_board.cellAtIndex(cellIndex);

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
        return m_board.typeName(cell.type);
    case SelectedRole:
        return cellIndex == m_selectedIndex;
    case MovableRole:
        return m_board.isMovable(cell);
    case LabelRole:
        return m_board.cellLabel(cell);
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
    return m_board.rows();
}

int BoardModel::columns() const
{
    return m_board.columns();
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
    return m_board.remainingBoxes();
}

int BoardModel::targetBoxes() const
{
    return m_board.targetBoxes();
}

quint32 BoardModel::seed() const
{
    return m_board.seed();
}

QString BoardModel::statusText() const
{
    return m_statusText;
}

void BoardModel::clickCell(int row, int column)
{
    if (m_inputLocked || !m_board.isInBounds(row, column)) {
        return;
    }

    const int clickedIndex = m_board.toCellIndex(row, column);
    const Match3::Cell &clickedCell = m_board.cellAtIndex(clickedIndex);

    if (!m_board.isMovable(clickedCell)) {
        if (m_board.isBox(clickedCell)) {
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
    m_board.reset(m_board.seed());
    m_selectedIndex = -1;
    m_inputLocked = false;
    m_gameWon = false;
    m_pendingSwap.clear();
    m_state = TurnState::Idle;
    m_chainCount = 0;
    m_statusText = QStringLiteral("Board reset. Same seed reproduces the same starting layout.");
    endResetModel();

    emit inputLockedChanged();
    emit gameWonChanged();
    emit remainingBoxesChanged();
    emit seedChanged();
    emit statusTextChanged();
}

void BoardModel::nextSeed()
{
    beginResetModel();
    m_board.reset(m_board.seed() + 1);
    m_selectedIndex = -1;
    m_inputLocked = false;
    m_gameWon = false;
    m_pendingSwap.clear();
    m_state = TurnState::Idle;
    m_chainCount = 0;
    m_statusText = QStringLiteral("Board advanced to the next seed.");
    endResetModel();

    emit inputLockedChanged();
    emit gameWonChanged();
    emit remainingBoxesChanged();
    emit seedChanged();
    emit statusTextChanged();
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
    if (m_board.cellCount() == 0) {
        return;
    }

    const QModelIndex first = QAbstractListModel::index(0, 0);
    const QModelIndex last = QAbstractListModel::index(m_board.cellCount() - 1, 0);
    emit dataChanged(first, last);
}

bool BoardModel::isAdjacent(int first, int second) const
{
    const QPoint firstPoint = m_board.fromCellIndex(first);
    const QPoint secondPoint = m_board.fromCellIndex(second);
    return std::abs(firstPoint.x() - secondPoint.x()) + std::abs(firstPoint.y() - secondPoint.y()) == 1;
}

bool BoardModel::canSwap(int first, int second) const
{
    return isAdjacent(first, second)
        && m_board.isMovable(m_board.cellAtIndex(first))
        && m_board.isMovable(m_board.cellAtIndex(second));
}

void BoardModel::startSwap(int first, int second)
{
    setSelectedIndex(-1);
    setInputLocked(true);
    m_chainCount = 0;
    setStatusText(QStringLiteral("Swapping..."));

    std::swap(m_board.cellAtIndex(first), m_board.cellAtIndex(second));
    m_pendingSwap = { first, second };
    m_state = TurnState::Swapping;
    notifyBoardChanged();

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        finishSwapEvaluation();
    });
}

void BoardModel::finishSwapEvaluation()
{
    if (m_pendingSwap.isValid()) {
        const Match3::Cell &firstCell = m_board.cellAtIndex(m_pendingSwap.first);
        const Match3::Cell &secondCell = m_board.cellAtIndex(m_pendingSwap.second);
        if (m_board.isSpecial(firstCell) || m_board.isSpecial(secondCell)) {
            const Match3::EffectResult effect = m_engine.createSpecialSwapEffect(&m_board, m_pendingSwap);
            if (!effect.isEmpty()) {
                resolveEffectResult(effect);
                return;
            }
        }
    }

    const Match3::MatchInfo matchInfo = m_engine.analyzeMatches(m_board, m_pendingSwap);
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

    m_state = TurnState::Reverting;
    std::swap(m_board.cellAtIndex(m_pendingSwap.first), m_board.cellAtIndex(m_pendingSwap.second));
    notifyBoardChanged();
    setStatusText(QStringLiteral("No valid match or tool trigger formed. Swap reverted."));

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        finishTurn();
    });
}

void BoardModel::processCascade()
{
    const Match3::MatchInfo matchInfo = m_engine.analyzeMatches(m_board, {});
    if (matchInfo.isEmpty()) {
        finishTurn();
        return;
    }

    resolveMatchInfo(matchInfo);
}

void BoardModel::finishTurn()
{
    m_pendingSwap.clear();
    m_state = TurnState::Idle;

    if (m_board.remainingBoxes() == 0) {
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

void BoardModel::resolveMatchInfo(const Match3::MatchInfo &matchInfo)
{
    ++m_chainCount;
    m_engine.resolveMatch(&m_board, matchInfo);

    m_state = TurnState::Clearing;
    if (matchInfo.spawns.isEmpty()) {
        setStatusText(QStringLiteral("Resolve round %1: clear matches, then fall and refill.").arg(m_chainCount));
    } else {
        setStatusText(QStringLiteral("Resolve round %1: clear matches and generate %2 tool(s).").arg(m_chainCount).arg(matchInfo.spawns.size()));
    }

    notifyBoardChanged();
    emit remainingBoxesChanged();

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        m_state = TurnState::Falling;
        m_board.applyGravity();
        m_board.refillEmptyCells();
        notifyBoardChanged();

        setStatusText(QStringLiteral("Board settled. Checking for another cascade..."));

        QTimer::singleShot(kAnimationDelayMs, this, [this]() {
            processCascade();
        });
    });
}

void BoardModel::resolveEffectResult(const Match3::EffectResult &effect)
{
    ++m_chainCount;
    m_engine.resolveEffect(&m_board, effect);

    m_state = TurnState::Clearing;
    setStatusText(effect.status.isEmpty()
        ? QStringLiteral("Special effect resolved.")
        : effect.status);
    notifyBoardChanged();
    emit remainingBoxesChanged();

    QTimer::singleShot(kAnimationDelayMs, this, [this]() {
        m_state = TurnState::Falling;
        m_board.applyGravity();
        m_board.refillEmptyCells();
        notifyBoardChanged();

        setStatusText(QStringLiteral("Board settled. Checking for another cascade..."));

        QTimer::singleShot(kAnimationDelayMs, this, [this]() {
            processCascade();
        });
    });
}
