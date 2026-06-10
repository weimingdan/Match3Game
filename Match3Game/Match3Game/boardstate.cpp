#include "boardstate.h"

#include <algorithm>
#include <array>

namespace
{
constexpr std::array<QPoint, 33> kInitialBoxPositions = {
    QPoint(0, 4), QPoint(1, 4), QPoint(2, 4), QPoint(6, 4), QPoint(7, 4), QPoint(8, 4),
    QPoint(0, 5), QPoint(1, 5), QPoint(2, 5), QPoint(3, 5), QPoint(4, 5), QPoint(5, 5), QPoint(6, 5), QPoint(7, 5), QPoint(8, 5),
    QPoint(0, 6), QPoint(1, 6), QPoint(2, 6), QPoint(3, 6), QPoint(4, 6), QPoint(5, 6), QPoint(6, 6), QPoint(7, 6), QPoint(8, 6),
    QPoint(0, 7), QPoint(1, 7), QPoint(2, 7), QPoint(3, 7), QPoint(4, 7), QPoint(5, 7), QPoint(6, 7), QPoint(7, 7), QPoint(8, 7)
};

bool isInitialBoxPosition(int row, int column)
{
    for (const QPoint &position : kInitialBoxPositions) {
        if (position.y() == row && position.x() == column) {
            return true;
        }
    }

    return false;
}
}

BoardState::BoardState()
    : m_rng(0xC0FFEEu)
{
    reset(m_seed);
}

void BoardState::reset(quint32 seed)
{
    m_seed = seed;
    initializeBoard();
}

int BoardState::rows() const
{
    return kRows;
}

int BoardState::columns() const
{
    return kColumns;
}

int BoardState::cellCount() const
{
    return m_cells.size();
}

quint32 BoardState::seed() const
{
    return m_seed;
}

int BoardState::remainingBoxes() const
{
    return m_remainingBoxes;
}

int BoardState::targetBoxes() const
{
    return m_targetBoxes;
}

const QVector<Match3::Cell> &BoardState::cells() const
{
    return m_cells;
}

const Match3::Cell &BoardState::cellAt(int row, int column) const
{
    return m_cells[toCellIndex(row, column)];
}

Match3::Cell &BoardState::cellAt(int row, int column)
{
    return m_cells[toCellIndex(row, column)];
}

const Match3::Cell &BoardState::cellAtIndex(int index) const
{
    return m_cells[index];
}

Match3::Cell &BoardState::cellAtIndex(int index)
{
    return m_cells[index];
}

bool BoardState::isInBounds(int row, int column) const
{
    return row >= 0 && row < kRows && column >= 0 && column < kColumns;
}

int BoardState::toCellIndex(int row, int column) const
{
    return row * kColumns + column;
}

QPoint BoardState::fromCellIndex(int index) const
{
    return QPoint(index % kColumns, index / kColumns);
}

bool BoardState::isEmptyCell(const Match3::Cell &cell) const
{
    return cell.type == Match3::ItemType::Empty;
}

bool BoardState::isBox(const Match3::Cell &cell) const
{
    return cell.type == Match3::ItemType::Box;
}

bool BoardState::isRocket(const Match3::Cell &cell) const
{
    return cell.type == Match3::ItemType::RocketHorizontal || cell.type == Match3::ItemType::RocketVertical;
}

bool BoardState::isSpecial(const Match3::Cell &cell) const
{
    return isRocket(cell) || cell.type == Match3::ItemType::Bomb || cell.type == Match3::ItemType::Propeller;
}

bool BoardState::isMovable(const Match3::Cell &cell) const
{
    return cell.type == Match3::ItemType::Normal || isSpecial(cell);
}

bool BoardState::isMatchable(const Match3::Cell &cell) const
{
    return cell.type == Match3::ItemType::Normal && cell.colorId >= 0;
}

bool BoardState::clearBoxAt(int index)
{
    if (index < 0 || index >= m_cells.size() || !isBox(m_cells[index])) {
        return false;
    }

    m_cells[index] = {};
    m_remainingBoxes = std::max(0, m_remainingBoxes - 1);
    return true;
}

bool BoardState::clearMovableAt(int index)
{
    if (index < 0 || index >= m_cells.size()) {
        return false;
    }

    Match3::Cell &cell = m_cells[index];
    if (cell.type == Match3::ItemType::Empty || cell.type == Match3::ItemType::Box) {
        return false;
    }

    cell = {};
    return true;
}

bool BoardState::applyGravity()
{
    bool moved = false;
    while (applyGravityPass()) {
        moved = true;
    }
    return moved;
}

bool BoardState::refillEmptyCells()
{
    bool filled = false;

    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            Match3::Cell &cell = cellAt(row, column);
            if (!isEmptyCell(cell)) {
                continue;
            }

            cell = randomNormalCell();
            filled = true;
        }
    }

    return filled;
}

int BoardState::chooseTopLayerBox()
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

QString BoardState::typeName(Match3::ItemType type) const
{
    switch (type) {
    case Match3::ItemType::Empty:
        return QStringLiteral("empty");
    case Match3::ItemType::Normal:
        return QStringLiteral("normal");
    case Match3::ItemType::Box:
        return QStringLiteral("box");
    case Match3::ItemType::RocketHorizontal:
        return QStringLiteral("rocketHorizontal");
    case Match3::ItemType::RocketVertical:
        return QStringLiteral("rocketVertical");
    case Match3::ItemType::Bomb:
        return QStringLiteral("bomb");
    case Match3::ItemType::Propeller:
        return QStringLiteral("propeller");
    }

    return QStringLiteral("unknown");
}

QString BoardState::cellLabel(const Match3::Cell &cell) const
{
    switch (cell.type) {
    case Match3::ItemType::Empty:
        return QStringLiteral("");
    case Match3::ItemType::Normal:
        return QString::number(cell.colorId + 1);
    case Match3::ItemType::Box:
        return QStringLiteral("BOX");
    case Match3::ItemType::RocketHorizontal:
        return QStringLiteral("ROW");
    case Match3::ItemType::RocketVertical:
        return QStringLiteral("COL");
    case Match3::ItemType::Bomb:
        return QStringLiteral("BOMB");
    case Match3::ItemType::Propeller:
        return QStringLiteral("PROP");
    }

    return QString();
}

void BoardState::initializeBoard()
{
    m_cells.resize(kRows * kColumns);
    std::fill(m_cells.begin(), m_cells.end(), Match3::Cell {});
    m_rng.seed(m_seed);

    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            Match3::Cell &cell = cellAt(row, column);
            if (isInitialBoxPosition(row, column)) {
                cell = { Match3::ItemType::Box, -1 };
                continue;
            }

            int colorId = randomColorId();
            int guard = 0;
            while (createsImmediateMatch(row, column, colorId) && guard < 20) {
                colorId = randomColorId();
                ++guard;
            }

            cell = { Match3::ItemType::Normal, colorId };
        }
    }

    m_remainingBoxes = 0;
    for (const Match3::Cell &cell : std::as_const(m_cells)) {
        if (cell.type == Match3::ItemType::Box) {
            ++m_remainingBoxes;
        }
    }

    m_targetBoxes = m_remainingBoxes;
}

bool BoardState::applyGravityPass()
{
    bool moved = false;

    for (int row = kRows - 1; row >= 0; --row) {
        for (int column = 0; column < kColumns; ++column) {
            Match3::Cell &targetCell = cellAt(row, column);
            if (!isEmptyCell(targetCell)) {
                continue;
            }

            const int aboveRow = row - 1;
            if (aboveRow >= 0) {
                Match3::Cell &aboveCell = cellAt(aboveRow, column);
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

int BoardState::chooseDiagonalSource(int row, int column)
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

bool BoardState::canSlideDiagonally(int sourceRow, int sourceColumn) const
{
    const int belowRow = sourceRow + 1;
    if (!isInBounds(belowRow, sourceColumn)) {
        return false;
    }

    return !isEmptyCell(cellAt(belowRow, sourceColumn));
}

int BoardState::randomColorId()
{
    return m_rng.bounded(kColorCount);
}

Match3::Cell BoardState::randomNormalCell()
{
    return { Match3::ItemType::Normal, randomColorId() };
}

bool BoardState::createsImmediateMatch(int row, int column, int colorId) const
{
    if (column >= 2) {
        const Match3::Cell &left = cellAt(row, column - 1);
        const Match3::Cell &leftLeft = cellAt(row, column - 2);
        if (isMatchable(left) && isMatchable(leftLeft)
            && left.colorId == colorId && leftLeft.colorId == colorId) {
            return true;
        }
    }

    if (row >= 2) {
        const Match3::Cell &up = cellAt(row - 1, column);
        const Match3::Cell &upUp = cellAt(row - 2, column);
        if (isMatchable(up) && isMatchable(upUp)
            && up.colorId == colorId && upUp.colorId == colorId) {
            return true;
        }
    }

    return false;
}
