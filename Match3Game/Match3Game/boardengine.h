#pragma once

#include "boardstate.h"

class BoardEngine
{
public:
    Match3::MatchInfo analyzeMatches(const BoardState &board, const Match3::SwapPair &pendingSwap) const;
    Match3::EffectResult createSpecialSwapEffect(BoardState *board, const Match3::SwapPair &pendingSwap) const;
    void resolveMatch(BoardState *board, const Match3::MatchInfo &matchInfo) const;
    void resolveEffect(BoardState *board, const Match3::EffectResult &effect) const;

private:
    QVector<Match3::LineGroup> findLineGroups(const BoardState &board, bool horizontal) const;
    QVector<QVector<int>> findSquareGroups(const BoardState &board) const;
    int chooseSpawnIndex(const QVector<int> &candidates, const QVector<int> &preferredOrder, const QVector<int> &reservedCells) const;
    void addSpawnRequest(QVector<Match3::SpawnRequest> *spawns, const Match3::SpawnRequest &request) const;

    void clearMatchedCells(BoardState *board, const Match3::MatchInfo &matchInfo) const;
    void placeGeneratedSpecials(BoardState *board, const QVector<Match3::SpawnRequest> &spawns) const;
    void clearAdjacentBoxes(BoardState *board, const QVector<int> &hitCells) const;

    void addLineEffect(const BoardState &board, int row, int column, bool horizontal, QVector<int> *hitCells, QVector<int> *adjacentBoxHits, QVector<int> *directBoxHits, bool allowAdjacentBoxHits) const;
    void addRocketEffect(const BoardState &board, int index, QVector<int> *hitCells, QVector<int> *adjacentBoxHits, QVector<int> *directBoxHits) const;
    void addBombEffect(const BoardState &board, int index, QVector<int> *hitCells, QVector<int> *directBoxHits) const;
    void addPropellerEffect(BoardState *board, int index, bool allowBoxTarget, QVector<int> *hitCells, QVector<int> *directBoxHits) const;
    int findOppositeRocketTarget(const BoardState &board, int rocketIndex) const;
};
