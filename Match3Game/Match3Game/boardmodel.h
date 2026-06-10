#pragma once

#include "boardengine.h"

#include <QAbstractListModel>
#include <QString>

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
    enum class TurnState {
        Idle,
        Swapping,
        Reverting,
        Clearing,
        Falling
    };

    void setSelectedIndex(int index);
    void setInputLocked(bool locked);
    void setStatusText(const QString &text);
    void notifyBoardChanged();

    bool isAdjacent(int first, int second) const;
    bool canSwap(int first, int second) const;

    void startSwap(int first, int second);
    void finishSwapEvaluation();
    void revertSwap();
    void processCascade();
    void finishTurn();
    void markVictory();
    void resolveMatchInfo(const Match3::MatchInfo &matchInfo);
    void resolveEffectResult(const Match3::EffectResult &effect);

    static constexpr int kAnimationDelayMs = 170;

    BoardState m_board;
    BoardEngine m_engine;
    int m_selectedIndex = -1;
    bool m_inputLocked = false;
    bool m_gameWon = false;
    QString m_statusText;
    Match3::SwapPair m_pendingSwap;
    TurnState m_state = TurnState::Idle;
    int m_chainCount = 0;
};
