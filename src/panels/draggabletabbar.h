#ifndef DRAGGABLETABBAR_H
#define DRAGGABLETABBAR_H

#include <QColor>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QTabBar>

class QTabWidget;

// ── Shared helpers used by both DraggableTabBar and SplitView ──

extern const char* MIME_TAB;

QString dragResponsePath(qint64 pid, quint64 dragId);
QColor dropHighlightColor();

// Returns a reference to the set of drag IDs already handled in this process.
// Used to prevent duplicate handling of same-process tab drags.
inline QSet<quint64>& handledDragIds()
{
    static QSet<quint64> ids;
    return ids;
}

// ── DraggableTabBar ──────────────────────────────────────────────

class DraggableTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DraggableTabBar(QWidget* parent = nullptr);
    void setDropIndicator(int index);
    void setDropIndicatorColor(const QColor& color);

signals:
    void tabDragStarted(int index);
    void tabDroppedOutside(int index, const QString& filePath);
    void closeTabRequested(int tabIndex);
    void closeOtherTabsRequested(int tabIndex);
    void closeTabsToRightRequested(int tabIndex);
    void closeAllTabsRequested();
    void copyPathRequested(int tabIndex);
    void copyFileNameRequested(int tabIndex);
    void showInFileManagerRequested(int tabIndex);
    void openInTerminalRequested(int tabIndex);
    void toggleTabPinnedRequested(int tabIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QPoint m_dragStartPos;
    int m_dragTabIndex = -1;
    bool m_dragging = false;
    int m_dropIndicatorIndex = -1;
    QColor m_dropColor;
};

#endif // DRAGGABLETABBAR_H
