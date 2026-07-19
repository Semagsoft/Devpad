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

#include "draggabletabbar.h"

#include <QMap>
#include <QPointer>
#include <QWidget>
#include <functional>

class DropZoneOverlay;
class QSplitter;
class QTabWidget;

class SplitView : public QWidget
{
    Q_OBJECT

public:
    enum class DropZone
    {
        None,
        Left,
        Right,
        Top,
        Bottom,
        Center
    };

    explicit SplitView(QWidget* parent = nullptr);
    ~SplitView() override;

    QTabWidget* activeTabWidget() const;
    QTabWidget* primaryTabWidget() const
    {
        return m_primaryWidget;
    }
    QTabWidget* addNewPane(QTabWidget* relativeTo, Qt::Orientation orientation, bool after);
    void removePane(QTabWidget* tabWidget);
    bool moveTabToPane(int tabIndex, QTabWidget* source, QTabWidget* target, int insertIndex = -1);
    void detachTabToWindow(int tabIndex, QTabWidget* source);

    static quint64 nextDragId();
    static void registerDragSource(quint64 id, QTabWidget* widget);
    static void removeDragSource(quint64 id);
    static QTabWidget* dragSource(quint64 id);

    int paneCount() const
    {
        return m_panes.size();
    }
    QTabWidget* paneAt(int index) const;
    QSplitter* splitter() const
    {
        return m_splitter;
    }

    void setPaneCallbacks(std::function<void(QTabWidget*)> onAdded, std::function<void(QTabWidget*)> onRemoved,
                          std::function<void(QTabWidget*, int)> onTabMoved = nullptr);

    // Split current active pane
    void splitActivePane(Qt::Orientation orientation);

signals:
    void activeTabWidgetChanged(QTabWidget* tabWidget);
    void tabDetachedToWindow(const QString& filePath);
    void closeAllTabsRequested();
    void tabPinToggled(int tabIndex, QTabWidget* pane);
    void externalTabDropped(const QString& filePath);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTabWidgetCurrentChanged(int index);

private:
    QTabWidget* createTabWidget();
    QTabWidget* findPaneAt(QPoint screenPos) const;
    QTabBar* tabBarAt(QPoint screenPos) const;
    QTabWidget* tabWidgetForBar(QTabBar* bar) const;
    void installFilterOnChildWidgets(QWidget* widget);
    DropZoneOverlay* overlay();
    void handleDrop(QDropEvent* event);
    int totalTabCount() const;

    // Nested splitter helpers
    QSplitter* parentSplitterFor(QWidget* widget) const;
    void splitPane(QTabWidget* pane, DropZone zone, QTabWidget* sourcePane, int sourceIndex);
    void collapseSplitter(QSplitter* splitter);
    void syncPaneList();
    void distributeSplitter(QSplitter* splitter);
    void setupNewPane(QTabWidget* newPane);

    // Zone detection
    DropZone calcDropZone(QTabWidget* pane, QPoint screenPos) const;
    void updateDropOverlay(QPoint screenPos);
    void clearDropIndicators();

    QSplitter* m_splitter = nullptr;
    DropZoneOverlay* m_dropOverlay = nullptr;
    QTabWidget* m_primaryWidget = nullptr;
    QList<QTabWidget*> m_panes;
    QTabWidget* m_activeWidget = nullptr;
    bool m_dragActive = false;

    // highlight state for tab bar
    QPointer<QTabBar> m_highlightBar;
    int m_highlightIndex = -1;

    // drop zone state
    DropZone m_activeDropZone = DropZone::None;
    QTabWidget* m_dropTargetPane = nullptr;

    bool m_dropping = false;

    std::function<void(QTabWidget*)> m_onPaneAdded;
    std::function<void(QTabWidget*)> m_onPaneRemoved;
    std::function<void(QTabWidget*, int)> m_onTabMoved;

    static quint64 s_dragIdCounter;
    static QMap<quint64, QPointer<QTabWidget>> s_dragSourceMap;
    static constexpr int MAX_PANES = 4;
    friend class DraggableTabBar;
};

#endif
