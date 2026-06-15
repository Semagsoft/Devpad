/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef SPLITVIEW_H
#define SPLITVIEW_H

#include <QWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QSplitter>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QApplication>
#include <QPointer>
#include <QMap>
#include <QContextMenuEvent>
#include <functional>

class DraggableTabBar : public QTabBar {
    Q_OBJECT

public:
    explicit DraggableTabBar(QWidget *parent = nullptr);
    void setDropIndicator(int index);
    void setDropIndicatorColor(const QColor &color);

signals:
    void tabDragStarted(int index);
    void tabDroppedOutside(int index, const QString &filePath);
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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_dragStartPos;
    int m_dragTabIndex = -1;
    bool m_dragging = false;
    int m_dropIndicatorIndex = -1;
    QColor m_dropColor;
};

class SplitView : public QWidget {
    Q_OBJECT

public:
    explicit SplitView(QWidget *parent = nullptr);
    ~SplitView() override;

    QTabWidget* activeTabWidget() const;
    QTabWidget* primaryTabWidget() const { return m_primaryWidget; }
    QTabWidget* addNewPane(Qt::Orientation orientation, int index = -1);
    void removePane(QTabWidget *tabWidget);
    bool moveTabToPane(int tabIndex, QTabWidget *source, QTabWidget *target, int insertIndex = -1);
    void detachTabToWindow(int tabIndex, QTabWidget *source);

    static quint64 nextDragId();
    static void registerDragSource(quint64 id, QTabWidget *widget);
    static QTabWidget* dragSource(quint64 id);

    int paneCount() const { return m_panes.size(); }
    QTabWidget* paneAt(int index) const;
    QSplitter* splitter() const { return m_splitter; }

    void setPaneCallbacks(std::function<void(QTabWidget*)> onAdded,
                          std::function<void(QTabWidget*)> onRemoved,
                          std::function<void(QTabWidget*, int)> onTabMoved = nullptr);

signals:
    void activeTabWidgetChanged(QTabWidget *tabWidget);
    void tabDetachedToWindow(const QString &filePath);
    void closeAllTabsRequested();
    void tabPinToggled(int tabIndex, QTabWidget *pane);
    void externalTabDropped(const QString &filePath);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTabWidgetCurrentChanged(int index);

private:
    QTabWidget* createTabWidget();
    QTabWidget* findPaneAt(QPoint screenPos) const;
    QTabWidget* tabWidgetForBar(QTabBar *bar) const;
    void installFilterOnChildWidgets(QWidget *widget);
    void handleDrop(QDropEvent *event);
    int totalTabCount() const;

    QSplitter *m_splitter;
    QWidget *m_dropOverlay;
    QTabWidget *m_primaryWidget;
    QList<QTabWidget*> m_panes;
    QTabWidget *m_activeWidget = nullptr;
    bool m_dragActive = false;

    // highlight state
    QPointer<QTabBar> m_highlightBar;
    int m_highlightIndex = -1;

    bool m_dropping = false;

    std::function<void(QTabWidget*)> m_onPaneAdded;
    std::function<void(QTabWidget*)> m_onPaneRemoved;
    std::function<void(QTabWidget*, int)> m_onTabMoved;

    static quint64 s_dragIdCounter;
    static QMap<quint64, QPointer<QTabWidget>> s_dragSourceMap;
    friend class DraggableTabBar;
};

#endif
