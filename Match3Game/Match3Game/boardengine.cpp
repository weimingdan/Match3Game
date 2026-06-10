#include "boardengine.h"

#include <QSet>

#include <algorithm>

namespace
{
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

Match3::MatchInfo BoardEngine::analyzeMatches(const BoardState &board, const Match3::SwapPair &pendingSwap) const
{
    Match3::MatchInfo info;

    const QVector<Match3::LineGroup> horizontalGroups = findLineGroups(board, true);
    const QVector<Match3::LineGroup> verticalGroups = findLineGroups(board, false);
    const QVector<QVector<int>> squareGroups = findSquareGroups(board);

    QSet<int> matchedSet;
    for (const Match3::LineGroup &group : horizontalGroups) {
        for (const int index : group.cells) {
            matchedSet.insert(index);
        }
    }
    for (const Match3::LineGroup &group : verticalGroups) {
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

    QVector<Match3::SpawnRequest> spawns;
    QVector<int> reservedCells;
    const QVector<int> preferredOrder = { pendingSwap.second, pendingSwap.first };

    for (const Match3::LineGroup &horizontal : horizontalGroups) {
        for (const Match3::LineGroup &vertical : verticalGroups) {
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
                addSpawnRequest(&spawns, { spawnIndex, Match3::ItemType::Bomb, 3 });
            }

            reservedCells += unionSet.values().toVector();
        }
    }

    for (const Match3::LineGroup &group : horizontalGroups) {
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
            addSpawnRequest(&spawns, { spawnIndex, Match3::ItemType::RocketVertical, 2 });
            reservedCells += group.cells;
        }
    }

    for (const Match3::LineGroup &group : verticalGroups) {
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
            addSpawnRequest(&spawns, { spawnIndex, Match3::ItemType::RocketHorizontal, 2 });
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
            addSpawnRequest(&spawns, { spawnIndex, Match3::ItemType::Propeller, 1 });
            reservedCells += square;
        }
    }

    info.spawns = spawns;
    return info;
}

Match3::EffectResult BoardEngine::createSpecialSwapEffect(BoardState *board, const Match3::SwapPair &pendingSwap) const
{
    Match3::EffectResult effect;
    if (!pendingSwap.isValid()) {
        return effect;
    }

    const int firstIndex = pendingSwap.first;
    const int secondIndex = pendingSwap.second;
    const Match3::Cell &firstCell = board->cellAtIndex(firstIndex);
    const Match3::Cell &secondCell = board->cellAtIndex(secondIndex);

    const bool firstIsPropeller = firstCell.type == Match3::ItemType::Propeller;
    const bool secondIsPropeller = secondCell.type == Match3::ItemType::Propeller;
    const bool firstIsRocket = board->isRocket(firstCell);
    const bool secondIsRocket = board->isRocket(secondCell);

    if ((firstIsPropeller && secondIsRocket) || (secondIsPropeller && firstIsRocket)) {
        const int propellerIndex = firstIsPropeller ? firstIndex : secondIndex;
        const int rocketIndex = firstIsRocket ? firstIndex : secondIndex;

        effect.hitCells.push_back(propellerIndex);
        effect.hitCells.push_back(rocketIndex);

        const int oppositeRocketIndex = findOppositeRocketTarget(*board, rocketIndex);
        if (oppositeRocketIndex >= 0) {
            const QPoint oppositePoint = board->fromCellIndex(oppositeRocketIndex);
            addLineEffect(*board, oppositePoint.y(), oppositePoint.x(), true, &effect.hitCells, &effect.adjacentBoxHits, &effect.directBoxHits, false);
            addLineEffect(*board, oppositePoint.y(), oppositePoint.x(), false, &effect.hitCells, &effect.adjacentBoxHits, &effect.directBoxHits, false);
            effect.status = QStringLiteral("Rocket + propeller combo: cross clear fires from the opposite rocket.");
        } else {
            addRocketEffect(*board, rocketIndex, &effect.hitCells, &effect.adjacentBoxHits, &effect.directBoxHits);
            effect.status = QStringLiteral("Rocket + propeller combo fell back to the swapped rocket effect.");
        }

        return effect;
    }

    QVector<int> specialIndices;
    if (board->isSpecial(firstCell)) {
        specialIndices.push_back(firstIndex);
    }
    if (board->isSpecial(secondCell)) {
        specialIndices.push_back(secondIndex);
    }

    for (const int index : uniqueSorted(specialIndices)) {
        const Match3::Cell &cell = board->cellAtIndex(index);
        if (board->isRocket(cell)) {
            addRocketEffect(*board, index, &effect.hitCells, &effect.adjacentBoxHits, &effect.directBoxHits);
        } else if (cell.type == Match3::ItemType::Bomb) {
            addBombEffect(*board, index, &effect.hitCells, &effect.directBoxHits);
        } else if (cell.type == Match3::ItemType::Propeller) {
            addPropellerEffect(board, index, true, &effect.hitCells, &effect.directBoxHits);
        }
    }

    effect.status = QStringLiteral("Special item activated by swap.");
    return effect;
}

void BoardEngine::resolveMatch(BoardState *board, const Match3::MatchInfo &matchInfo) const
{
    clearMatchedCells(board, matchInfo);
}

void BoardEngine::resolveEffect(BoardState *board, const Match3::EffectResult &effect) const
{
    const QVector<int> hitCells = uniqueSorted(effect.hitCells);
    const QVector<int> adjacentBoxes = uniqueSorted(effect.adjacentBoxHits);
    const QVector<int> directBoxes = uniqueSorted(effect.directBoxHits);

    for (const int index : hitCells) {
        board->clearMovableAt(index);
    }

    QSet<int> allBoxes(adjacentBoxes.begin(), adjacentBoxes.end());
    for (const int boxIndex : directBoxes) {
        allBoxes.insert(boxIndex);
    }

    for (const int boxIndex : allBoxes) {
        board->clearBoxAt(boxIndex);
    }
}

QVector<Match3::LineGroup> BoardEngine::findLineGroups(const BoardState &board, bool horizontal) const
{
    QVector<Match3::LineGroup> groups;

    const int outerLimit = horizontal ? board.rows() : board.columns();
    const int innerLimit = horizontal ? board.columns() : board.rows();

    for (int outer = 0; outer < outerLimit; ++outer) {
        int inner = 0;
        while (inner < innerLimit) {
            const int row = horizontal ? outer : inner;
            const int column = horizontal ? inner : outer;
            const Match3::Cell &startCell = board.cellAt(row, column);

            if (!board.isMatchable(startCell)) {
                ++inner;
                continue;
            }

            int end = inner + 1;
            while (end < innerLimit) {
                const int candidateRow = horizontal ? outer : end;
                const int candidateColumn = horizontal ? end : outer;
                const Match3::Cell &candidate = board.cellAt(candidateRow, candidateColumn);
                if (!board.isMatchable(candidate) || candidate.colorId != startCell.colorId) {
                    break;
                }
                ++end;
            }

            if (end - inner >= 3) {
                Match3::LineGroup group;
                group.horizontal = horizontal;
                group.colorId = startCell.colorId;

                for (int current = inner; current < end; ++current) {
                    const int groupRow = horizontal ? outer : current;
                    const int groupColumn = horizontal ? current : outer;
                    group.cells.push_back(board.toCellIndex(groupRow, groupColumn));
                }

                groups.push_back(group);
            }

            inner = end;
        }
    }

    return groups;
}

QVector<QVector<int>> BoardEngine::findSquareGroups(const BoardState &board) const
{
    QVector<QVector<int>> groups;

    for (int row = 0; row < board.rows() - 1; ++row) {
        for (int column = 0; column < board.columns() - 1; ++column) {
            const Match3::Cell &topLeft = board.cellAt(row, column);
            const Match3::Cell &topRight = board.cellAt(row, column + 1);
            const Match3::Cell &bottomLeft = board.cellAt(row + 1, column);
            const Match3::Cell &bottomRight = board.cellAt(row + 1, column + 1);

            if (!board.isMatchable(topLeft)) {
                continue;
            }

            const int colorId = topLeft.colorId;
            if (!board.isMatchable(topRight) || topRight.colorId != colorId
                || !board.isMatchable(bottomLeft) || bottomLeft.colorId != colorId
                || !board.isMatchable(bottomRight) || bottomRight.colorId != colorId) {
                continue;
            }

            groups.push_back({
                board.toCellIndex(row, column),
                board.toCellIndex(row, column + 1),
                board.toCellIndex(row + 1, column),
                board.toCellIndex(row + 1, column + 1)
            });
        }
    }

    return groups;
}

int BoardEngine::chooseSpawnIndex(const QVector<int> &candidates, const QVector<int> &preferredOrder, const QVector<int> &reservedCells) const
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

void BoardEngine::addSpawnRequest(QVector<Match3::SpawnRequest> *spawns, const Match3::SpawnRequest &request) const
{
    if (request.index < 0) {
        return;
    }

    for (Match3::SpawnRequest &existing : *spawns) {
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

void BoardEngine::clearMatchedCells(BoardState *board, const Match3::MatchInfo &matchInfo) const
{
    QVector<int> hitCells;
    QVector<int> spawnIndices;
    for (const Match3::SpawnRequest &spawn : matchInfo.spawns) {
        spawnIndices.push_back(spawn.index);
    }

    for (const int index : matchInfo.matchedCells) {
        if (index < 0 || index >= board->cellCount()) {
            continue;
        }

        if (containsValue(spawnIndices, index)) {
            hitCells.push_back(index);
            continue;
        }

        if (board->clearMovableAt(index)) {
            hitCells.push_back(index);
        }
    }

    clearAdjacentBoxes(board, hitCells);
    placeGeneratedSpecials(board, matchInfo.spawns);
}

void BoardEngine::placeGeneratedSpecials(BoardState *board, const QVector<Match3::SpawnRequest> &spawns) const
{
    for (const Match3::SpawnRequest &spawn : spawns) {
        if (spawn.index < 0 || spawn.index >= board->cellCount()) {
            continue;
        }

        board->cellAtIndex(spawn.index) = { spawn.type, -1 };
    }
}

void BoardEngine::clearAdjacentBoxes(BoardState *board, const QVector<int> &hitCells) const
{
    QVector<int> boxIndices;

    for (const int hitIndex : uniqueSorted(hitCells)) {
        if (hitIndex < 0 || hitIndex >= board->cellCount()) {
            continue;
        }

        const QPoint position = board->fromCellIndex(hitIndex);
        for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
            const int nextColumn = position.x() + offset.x();
            const int nextRow = position.y() + offset.y();
            if (!board->isInBounds(nextRow, nextColumn)) {
                continue;
            }

            const int candidateIndex = board->toCellIndex(nextRow, nextColumn);
            if (board->isBox(board->cellAtIndex(candidateIndex))) {
                boxIndices.push_back(candidateIndex);
            }
        }
    }

    for (const int boxIndex : uniqueSorted(boxIndices)) {
        board->clearBoxAt(boxIndex);
    }
}

void BoardEngine::addLineEffect(const BoardState &board, int row, int column, bool horizontal, QVector<int> *hitCells, QVector<int> *adjacentBoxHits, QVector<int> *directBoxHits, bool allowAdjacentBoxHits) const
{
    if (horizontal) {
        for (int currentColumn = 0; currentColumn < board.columns(); ++currentColumn) {
            const int index = board.toCellIndex(row, currentColumn);
            hitCells->push_back(index);
            if (board.isBox(board.cellAtIndex(index))) {
                directBoxHits->push_back(index);
            }

            if (allowAdjacentBoxHits) {
                for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
                    const int nextColumn = currentColumn + offset.x();
                    const int nextRow = row + offset.y();
                    if (!board.isInBounds(nextRow, nextColumn)) {
                        continue;
                    }

                    const int candidateIndex = board.toCellIndex(nextRow, nextColumn);
                    if (board.isBox(board.cellAtIndex(candidateIndex))) {
                        adjacentBoxHits->push_back(candidateIndex);
                    }
                }
            }
        }
    } else {
        for (int currentRow = 0; currentRow < board.rows(); ++currentRow) {
            const int index = board.toCellIndex(currentRow, column);
            hitCells->push_back(index);
            if (board.isBox(board.cellAtIndex(index))) {
                directBoxHits->push_back(index);
            }

            if (allowAdjacentBoxHits) {
                for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
                    const int nextColumn = column + offset.x();
                    const int nextRow = currentRow + offset.y();
                    if (!board.isInBounds(nextRow, nextColumn)) {
                        continue;
                    }

                    const int candidateIndex = board.toCellIndex(nextRow, nextColumn);
                    if (board.isBox(board.cellAtIndex(candidateIndex))) {
                        adjacentBoxHits->push_back(candidateIndex);
                    }
                }
            }
        }
    }
}

void BoardEngine::addRocketEffect(const BoardState &board, int index, QVector<int> *hitCells, QVector<int> *adjacentBoxHits, QVector<int> *directBoxHits) const
{
    if (index < 0 || index >= board.cellCount()) {
        return;
    }

    const QPoint position = board.fromCellIndex(index);
    const Match3::Cell &rocketCell = board.cellAtIndex(index);
    if (rocketCell.type == Match3::ItemType::RocketHorizontal) {
        addLineEffect(board, position.y(), position.x(), true, hitCells, adjacentBoxHits, directBoxHits, false);
    } else if (rocketCell.type == Match3::ItemType::RocketVertical) {
        addLineEffect(board, position.y(), position.x(), false, hitCells, adjacentBoxHits, directBoxHits, false);
    }
}

void BoardEngine::addBombEffect(const BoardState &board, int index, QVector<int> *hitCells, QVector<int> *directBoxHits) const
{
    if (index < 0 || index >= board.cellCount()) {
        return;
    }

    const QPoint center = board.fromCellIndex(index);
    for (int row = center.y() - 2; row <= center.y() + 2; ++row) {
        for (int column = center.x() - 2; column <= center.x() + 2; ++column) {
            if (!board.isInBounds(row, column)) {
                continue;
            }

            const int candidateIndex = board.toCellIndex(row, column);
            hitCells->push_back(candidateIndex);
            if (board.isBox(board.cellAtIndex(candidateIndex))) {
                directBoxHits->push_back(candidateIndex);
            }
        }
    }
}

void BoardEngine::addPropellerEffect(BoardState *board, int index, bool allowBoxTarget, QVector<int> *hitCells, QVector<int> *directBoxHits) const
{
    if (index < 0 || index >= board->cellCount()) {
        return;
    }

    const QPoint center = board->fromCellIndex(index);
    hitCells->push_back(index);

    for (const QPoint &offset : { QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1) }) {
        const int nextColumn = center.x() + offset.x();
        const int nextRow = center.y() + offset.y();
        if (!board->isInBounds(nextRow, nextColumn)) {
            continue;
        }

        const int candidateIndex = board->toCellIndex(nextRow, nextColumn);
        hitCells->push_back(candidateIndex);
    }

    if (!allowBoxTarget) {
        return;
    }

    const int targetBoxIndex = board->chooseTopLayerBox();
    if (targetBoxIndex >= 0) {
        directBoxHits->push_back(targetBoxIndex);
    }
}

int BoardEngine::findOppositeRocketTarget(const BoardState &board, int rocketIndex) const
{
    if (rocketIndex < 0 || rocketIndex >= board.cellCount()) {
        return -1;
    }

    const Match3::ItemType currentType = board.cellAtIndex(rocketIndex).type;
    const Match3::ItemType oppositeType = currentType == Match3::ItemType::RocketHorizontal
        ? Match3::ItemType::RocketVertical
        : Match3::ItemType::RocketHorizontal;

    for (int index = 0; index < board.cellCount(); ++index) {
        if (index == rocketIndex) {
            continue;
        }

        if (board.cellAtIndex(index).type == oppositeType) {
            return index;
        }
    }

    return -1;
}
