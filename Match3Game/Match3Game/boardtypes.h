#pragma once

#include <QString>
#include <QVector>

namespace Match3
{
enum class ItemType {
    Empty,
    Normal,
    Box,
    RocketHorizontal,
    RocketVertical,
    Bomb,
    Propeller
};

struct Cell {
    ItemType type = ItemType::Empty;
    int colorId = -1;
};

struct SwapPair {
    int first = -1;
    int second = -1;

    bool isValid() const
    {
        return first >= 0 && second >= 0;
    }

    void clear()
    {
        first = -1;
        second = -1;
    }
};

struct LineGroup {
    QVector<int> cells;
    bool horizontal = false;
    int colorId = -1;
};

struct SpawnRequest {
    int index = -1;
    ItemType type = ItemType::Empty;
    int priority = 0;
};

struct MatchInfo {
    QVector<int> matchedCells;
    QVector<SpawnRequest> spawns;

    bool isEmpty() const
    {
        return matchedCells.isEmpty();
    }
};

struct EffectResult {
    QVector<int> hitCells;
    QVector<int> adjacentBoxHits;
    QVector<int> directBoxHits;
    QString status;

    bool isEmpty() const
    {
        return hitCells.isEmpty() && adjacentBoxHits.isEmpty() && directBoxHits.isEmpty();
    }
};
}
