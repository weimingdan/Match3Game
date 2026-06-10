#pragma once

#include "boardtypes.h"

#include <QPoint>
#include <QRandomGenerator>
#include <QString>
#include <QtGlobal>
#include <QVector>

class BoardState
{
public:
    static constexpr int kRows = 8;
    static constexpr int kColumns = 9;
    static constexpr int kColorCount = 5;

    BoardState();

    void reset(quint32 seed);

    int rows() const;
    int columns() const;
    int cellCount() const;
    quint32 seed() const;
    int remainingBoxes() const;
    int targetBoxes() const;

    const QVector<Match3::Cell> &cells() const;
    const Match3::Cell &cellAt(int row, int column) const;
    Match3::Cell &cellAt(int row, int column);
    const Match3::Cell &cellAtIndex(int index) const;
    Match3::Cell &cellAtIndex(int index);

    bool isInBounds(int row, int column) const;
    int toCellIndex(int row, int column) const;
    QPoint fromCellIndex(int index) const;

    bool isEmptyCell(const Match3::Cell &cell) const;
    bool isBox(const Match3::Cell &cell) const;
    bool isRocket(const Match3::Cell &cell) const;
    bool isSpecial(const Match3::Cell &cell) const;
    bool isMovable(const Match3::Cell &cell) const;
    bool isMatchable(const Match3::Cell &cell) const;

    bool clearBoxAt(int index);
    bool clearMovableAt(int index);

    bool applyGravity();
    bool refillEmptyCells();
    int chooseTopLayerBox();

    QString typeName(Match3::ItemType type) const;
    QString cellLabel(const Match3::Cell &cell) const;

private:
    void initializeBoard();
    bool applyGravityPass();
    int chooseDiagonalSource(int row, int column);
    bool canSlideDiagonally(int sourceRow, int sourceColumn) const;
    int randomColorId();
    Match3::Cell randomNormalCell();
    bool createsImmediateMatch(int row, int column, int colorId) const;

    QVector<Match3::Cell> m_cells;
    QRandomGenerator m_rng;
    int m_remainingBoxes = 0;
    int m_targetBoxes = 0;
    quint32 m_seed = 0xC0FFEEu;
};
