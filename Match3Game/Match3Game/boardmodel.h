#pragma once

#include <QAbstractListModel>
#include <QPoint>
#include <QRandomGenerator>
#include <QString>
#include <QtGlobal>
#include <QVector>

class BoardModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int rows READ rows CONSTANT)
    Q_PROPERTY(int columns READ columns CONSTANT)
    Q_PROPERTY(bool inputLocked READ inputLocked NOTIFY inputLockedChanged)
    Q_PROPERTY(bool gameWon READ gameWon NOTIFY gameWonChanged)
    Q_PROPERTY(int remainingBoxes READ remainingBoxes NOTIFY remainingBoxesChanged)
    Q_PROPERTY(int targetBoxes READ targetBoxes CONSTANT)
    Q_PROPERTY(quint32 seed READ seed NOTIFY seedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    enum Role {
        CellRowRole = Qt::UserRole + 1,
        CellColumnRole,
        CellTypeRole,
        CellColorRole,
        CellTypeNameRole,
        SelectedRole,
        MovableRole,
        LabelRole
    };

    enum class ItemType {
        Empty,
        Normal,
        Box,
        RocketHorizontal,
        RocketVertical,
        Bomb,
        Propeller
    };
    Q_ENUM(ItemType)

    explicit BoardModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rows() const;
    int columns() const;
    bool inputLocked() const;
    bool gameWon() const;
    int remainingBoxes() const;
    int targetBoxes() const;
    quint32 seed() const;
    QString statusText() const;

    Q_INVOKABLE void clickCell(int row, int column);
    Q_INVOKABLE void resetBoard();
    Q_INVOKABLE void nextSeed();

signals:
    void inputLockedChanged();
    void gameWonChanged();
    void remainingBoxesChanged();
    void seedChanged();
    void statusTextChanged();

private:
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

    enum class State {
        Idle,
        Swapping,
        Reverting,
        Clearing,
        Falling
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

    void initializeBoard();
    void setSelectedIndex(int index);
    void setInputLocked(bool locked);
    void setStatusText(const QString &text);
    void notifyBoardChanged();

    bool isInBounds(int row, int column) const;
    int toCellIndex(int row, int column) const;
    QPoint fromCellIndex(int index) const;

    const Cell &cellAt(int row, int column) const;
    Cell &cellAt(int row, int column);

    bool isEmptyCell(const Cell &cell) const;
    bool isBox(const Cell &cell) const;
    bool isRocket(const Cell &cell) const;
    bool isSpecial(const Cell &cell) const;
    bool isMovable(const Cell &cell) const;
    bool isMatchable(const Cell &cell) const;
    bool isAdjacent(int first, int second) const;
    bool canSwap(int first, int second) const;

    void startSwap(int first, int second);
    void finishSwapEvaluation();
    void revertSwap();
    void processCascade();
    void finishTurn();
    void markVictory();

    MatchInfo analyzeMatches() const;
    QVector<LineGroup> findLineGroups(bool horizontal) const;
    QVector<QVector<int>> findSquareGroups() const;
    int chooseSpawnIndex(const QVector<int> &candidates, const QVector<int> &preferredOrder, const QVector<int> &reservedCells) const;
    void addSpawnRequest(QVector<SpawnRequest> *spawns, const SpawnRequest &request) const;
    void resolveMatchInfo(const MatchInfo &matchInfo);
    void clearMatchedCells(const MatchInfo &matchInfo);
    void placeGeneratedSpecials(const QVector<SpawnRequest> &spawns);

    void activateSpecialSwap();
    void activateSpecialItems(const QVector<int> &specialIndices, bool allowPropellerTarget, const QString &statusText);
    void resolveEffectResult(const EffectResult &effect);
    void addLineEffect(int row, int column, bool horizontal, QVector<int> *hitCells, QVector<int> *adjacentBoxHits, QVector<int> *directBoxHits, bool allowAdjacentBoxHits) const;
    void addRocketEffect(int index, QVector<int> *hitCells, QVector<int> *adjacentBoxHits, QVector<int> *directBoxHits) const;
    void addBombEffect(int index, QVector<int> *hitCells, QVector<int> *directBoxHits) const;
    void addPropellerEffect(int index, bool allowBoxTarget, QVector<int> *hitCells, QVector<int> *directBoxHits);
    int findOppositeRocketTarget(int rocketIndex) const;
    int chooseTopLayerBox();

    void clearAdjacentBoxes(const QVector<int> &hitCells);
    bool clearBoxAt(int index);
    bool clearMovableAt(int index);

    bool applyGravity();
    bool applyGravityPass();
    int chooseDiagonalSource(int row, int column);
    bool canSlideDiagonally(int sourceRow, int sourceColumn) const;
    bool refillEmptyCells();
    int randomColorId();
    Cell randomNormalCell();
    bool createsImmediateMatch(int row, int column, int colorId) const;

    QString typeName(ItemType type) const;
    QString cellLabel(const Cell &cell) const;

    static constexpr int kRows = 8;
    static constexpr int kColumns = 9;
    static constexpr int kAnimationDelayMs = 170;
    static constexpr int kColorCount = 5;

    QVector<Cell> m_cells;
    QRandomGenerator m_rng;
    int m_selectedIndex = -1;
    int m_remainingBoxes = 0;
    int m_targetBoxes = 0;
    bool m_inputLocked = false;
    bool m_gameWon = false;
    quint32 m_seed = 0xC0FFEEu;
    QString m_statusText;
    SwapPair m_pendingSwap;
    State m_state = State::Idle;
    int m_chainCount = 0;
};
