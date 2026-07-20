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
#include "splitview.h"

#include "appstrings.h"
#include "codeeditor.h"
#include "draggabletabbar.h"
#include "settingsmanager.h"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QLayout>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QResizeEvent>
#include <QSet>
#include <QSplitter>
#include <QStandardPaths>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QUrl>

quint64 SplitView::s_dragIdCounter = 0;
QMap<quint64, QPointer<QTabWidget>> SplitView::s_dragSourceMap;

quint64 SplitView::nextDragId()
{
    return ++s_dragIdCounter;
}
void SplitView::registerDragSource(quint64 id, QTabWidget* widget)
{
    s_dragSourceMap[id] = widget;
}
void SplitView::removeDragSource(quint64 id)
{
    s_dragSourceMap.remove(id);
}
QTabWidget* SplitView::dragSource(quint64 id)
{
    return s_dragSourceMap.value(id).data();
}

// ── DropZoneOverlay ───────────────────────────────────────────

class DropZoneOverlay : public QWidget
{
public:
    explicit DropZoneOverlay(QWidget* parent, QWidget* viewWidget) : QWidget(parent), m_viewWidget(viewWidget)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
        raise();
    }

    void setTargetPane(QTabWidget* pane, SplitView::DropZone zone, const QColor& color)
    {
        m_zone = zone;
        m_color = color;
        if (pane && zone != SplitView::DropZone::None)
        {
            QPoint topLeft = pane->mapTo(parentWidget(), QPoint(0, 0));
            m_paneRect = QRect(topLeft, pane->size());
        }
        else
        {
            m_paneRect = QRect();
        }
        repaint();
    }

    void clearZone()
    {
        m_zone = SplitView::DropZone::None;
        m_paneRect = QRect();
        repaint();
    }

    void updateOverlayPosition()
    {
        if (m_viewWidget)
        {
            QPoint tl = m_viewWidget->mapTo(parentWidget(), QPoint(0, 0));
            setGeometry(tl.x(), tl.y(), m_viewWidget->width(), m_viewWidget->height());
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (m_zone == SplitView::DropZone::None || !m_paneRect.isValid())
            return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect r = m_paneRect;
        QColor accent = m_color;

        if (m_zone == SplitView::DropZone::Center)
        {
            QColor fill = accent;
            fill.setAlpha(80);
            p.fillRect(r, fill);

            int m = std::min(r.width(), r.height()) / 5;
            QRect inner = r.adjusted(m, m, -m, -m);
            QColor border = accent;
            border.setAlpha(220);
            QPen pen(border, 4);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(inner, 10, 10);
            return;
        }

        QRect zoneRect;
        switch (m_zone)
        {
        case SplitView::DropZone::Left:
            zoneRect = QRect(r.left(), r.top(), r.width() / 3, r.height());
            break;
        case SplitView::DropZone::Right:
            zoneRect = QRect(r.right() - r.width() / 3, r.top(), r.width() / 3, r.height());
            break;
        case SplitView::DropZone::Top:
            zoneRect = QRect(r.left(), r.top(), r.width(), r.height() / 3);
            break;
        case SplitView::DropZone::Bottom:
            zoneRect = QRect(r.left(), r.bottom() - r.height() / 3, r.width(), r.height() / 3);
            break;
        default:
            return;
        }

        QColor fill = accent;
        fill.setAlpha(140);
        p.fillRect(zoneRect, fill);

        QColor line = accent;
        line.setAlpha(230);
        QPen linePen(line, 5);
        p.setPen(linePen);

        switch (m_zone)
        {
        case SplitView::DropZone::Left:
            p.drawLine(zoneRect.topRight(), zoneRect.bottomRight());
            break;
        case SplitView::DropZone::Right:
            p.drawLine(zoneRect.topLeft(), zoneRect.bottomLeft());
            break;
        case SplitView::DropZone::Top:
            p.drawLine(zoneRect.bottomLeft(), zoneRect.bottomRight());
            break;
        case SplitView::DropZone::Bottom:
            p.drawLine(zoneRect.topLeft(), zoneRect.topRight());
            break;
        default:
            break;
        }
    }

private:
    SplitView::DropZone m_zone = SplitView::DropZone::None;
    QRect m_paneRect;
    QColor m_color;
    QWidget* m_viewWidget = nullptr;
};

// ── TabWidget helper ────────────────────────────────────────────

class TabWidgetWithDraggableBar : public QTabWidget
{
public:
    explicit TabWidgetWithDraggableBar(QWidget* parent) : QTabWidget(parent)
    {
        auto* bar = new DraggableTabBar(this);
        setTabBar(bar);
    }
};

// ── SplitView ──────────────────────────────────────────────────

SplitView::SplitView(QWidget* parent) : QWidget(parent)
{
    setAcceptDrops(true);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->installEventFilter(this);

    m_primaryWidget = createTabWidget();
    m_splitter->addWidget(m_primaryWidget);
    m_panes.append(m_primaryWidget);
    m_activeWidget = m_primaryWidget;

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
}

SplitView::~SplitView() = default;

DropZoneOverlay* SplitView::overlay()
{
    if (!m_dropOverlay)
    {
        m_dropOverlay = new DropZoneOverlay(this, this);
        m_dropOverlay->hide();
    }
    return m_dropOverlay;
}

// ── Nested Splitter Helpers ────────────────────────────────────

QSplitter* SplitView::parentSplitterFor(QWidget* widget) const
{
    QWidget* parent = widget->parentWidget();
    while (parent)
    {
        if (auto* split = qobject_cast<QSplitter*>(parent))
            return split;
        if (parent == this)
            return nullptr;
        parent = parent->parentWidget();
    }
    return nullptr;
}

void SplitView::syncPaneList()
{
    m_panes.clear();
    std::function<void(QSplitter*)> collect = [&](QSplitter* split)
    {
        for (int i = 0; i < split->count(); ++i)
        {
            QWidget* w = split->widget(i);
            if (auto* tw = qobject_cast<QTabWidget*>(w))
            {
                m_panes.append(tw);
            }
            else if (auto* sub = qobject_cast<QSplitter*>(w))
            {
                collect(sub);
            }
        }
    };
    collect(m_splitter);
}

void SplitView::distributeSplitter(QSplitter* splitter)
{
    for (int i = 0; i < splitter->count(); ++i)
        splitter->setStretchFactor(i, 1);
}

void SplitView::setupNewPane(QTabWidget* newPane)
{
    if (m_onPaneAdded)
        m_onPaneAdded(newPane);
    syncPaneList();
    QSplitter* parentSplit = parentSplitterFor(newPane);
    distributeSplitter(parentSplit ? parentSplit : m_splitter);
}

void SplitView::splitPane(QTabWidget* pane, DropZone zone, QTabWidget* sourcePane, int sourceIndex)
{
    if (m_panes.size() >= MAX_PANES)
        return;

    Qt::Orientation orientation;
    bool insertBefore;

    switch (zone)
    {
    case DropZone::Left:
        orientation = Qt::Horizontal;
        insertBefore = true;
        break;
    case DropZone::Right:
        orientation = Qt::Horizontal;
        insertBefore = false;
        break;
    case DropZone::Top:
        orientation = Qt::Vertical;
        insertBefore = true;
        break;
    case DropZone::Bottom:
        orientation = Qt::Vertical;
        insertBefore = false;
        break;
    default:
        return;
    }

    // Create the new tab widget
    auto* newPane = createTabWidget();

    if (sourcePane)
    {
        QWidget* tabW = sourcePane->widget(sourceIndex);
        QString text = sourcePane->tabText(sourceIndex);
        QIcon icon = sourcePane->tabIcon(sourceIndex);
        QString tooltip = sourcePane->tabToolTip(sourceIndex);
        sourcePane->removeTab(sourceIndex);
        int newIdx = newPane->addTab(tabW, icon, text);
        newPane->setTabToolTip(newIdx, tooltip);
        newPane->setCurrentWidget(tabW);
        tabW->setFocus();
        if (m_onTabMoved)
            m_onTabMoved(newPane, newIdx);
    }

    QSplitter* parentSplit = parentSplitterFor(pane);
    if (!parentSplit)
        parentSplit = m_splitter;

    if (parentSplit->orientation() == orientation)
    {
        int idx = parentSplit->indexOf(pane);
        if (!insertBefore)
            idx++;
        parentSplit->insertWidget(idx, newPane);
        distributeSplitter(parentSplit);
    }
    else
    {
        int idx = parentSplit->indexOf(pane);
        auto* subSplit = new QSplitter(orientation);
        subSplit->setChildrenCollapsible(false);
        parentSplit->insertWidget(idx, subSplit);
        if (insertBefore)
        {
            subSplit->addWidget(newPane);
            subSplit->addWidget(pane);
        }
        else
        {
            subSplit->addWidget(pane);
            subSplit->addWidget(newPane);
        }
        distributeSplitter(subSplit);
    }

    setupNewPane(newPane);
}

void SplitView::collapseSplitter(QSplitter* splitter)
{
    if (!splitter || splitter == m_splitter)
        return;

    auto* parentSplit = qobject_cast<QSplitter*>(splitter->parentWidget());
    if (!parentSplit)
        return;

    int idx = parentSplit->indexOf(splitter);

    if (splitter->count() == 1)
    {
        // Replace the splitter with its sole child
        QWidget* child = splitter->widget(0);
        parentSplit->replaceWidget(idx, child);
        splitter->deleteLater();
    }
    else if (splitter->count() == 0)
    {
        // Empty splitter - remove it by replacing with a dummy that gets deleted
        auto* dummy = new QWidget();
        dummy->hide();
        parentSplit->replaceWidget(idx, dummy);
        splitter->deleteLater();
        dummy->deleteLater();
    }
}

// ── Zone Detection ─────────────────────────────────────────────

SplitView::DropZone SplitView::calcDropZone(QTabWidget* pane, QPoint screenPos) const
{
    QPoint localPos = pane->mapFromGlobal(screenPos);
    QSize size = pane->size();

    if (size.width() <= 0 || size.height() <= 0)
        return DropZone::None;

    double relX = static_cast<double>(localPos.x()) / size.width();
    double relY = static_cast<double>(localPos.y()) / size.height();

    // Edge threshold: 33% from each edge
    if (relX < 0.33)
        return DropZone::Left;
    if (relX > 0.67)
        return DropZone::Right;
    if (relY < 0.33)
        return DropZone::Top;
    if (relY > 0.67)
        return DropZone::Bottom;

    return DropZone::Center;
}

void SplitView::updateDropOverlay(QPoint screenPos)
{
    QTabWidget* pane = findPaneAt(screenPos);
    if (!pane)
    {
        clearDropIndicators();
        return;
    }

    DropZone zone = calcDropZone(pane, screenPos);

    if (pane != m_dropTargetPane || zone != m_activeDropZone)
    {
        m_dropTargetPane = pane;
        m_activeDropZone = zone;
        QColor c = dropHighlightColor();
        overlay()->setGeometry(rect());
        overlay()->setTargetPane(pane, zone, c);
        overlay()->show();
        overlay()->raise();
    }
}

void SplitView::clearDropIndicators()
{
    m_activeDropZone = DropZone::None;
    m_dropTargetPane = nullptr;
    if (m_highlightBar)
    {
        if (auto* oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
            oldDrag->setDropIndicator(-1);
        m_highlightBar = nullptr;
    }
    m_highlightIndex = -1;
    overlay()->clearZone();
    overlay()->hide();
}

// ── Split Active Pane ──────────────────────────────────────────

void SplitView::splitActivePane(Qt::Orientation orientation)
{
    if (!m_activeWidget || m_panes.size() >= MAX_PANES)
        return;

    auto* newPane = createTabWidget();

    QSplitter* parentSplit = parentSplitterFor(m_activeWidget);
    if (!parentSplit)
        parentSplit = m_splitter;

    if (parentSplit->orientation() == orientation)
    {
        int idx = parentSplit->indexOf(m_activeWidget) + 1;
        parentSplit->insertWidget(idx, newPane);
        distributeSplitter(parentSplit);
    }
    else
    {
        int idx = parentSplit->indexOf(m_activeWidget);
        auto* subSplit = new QSplitter(orientation);
        subSplit->setChildrenCollapsible(false);
        parentSplit->insertWidget(idx, subSplit);
        subSplit->addWidget(m_activeWidget);
        subSplit->addWidget(newPane);
        distributeSplitter(subSplit);
    }

    setupNewPane(newPane);
}

// ── Tab Widget Creation ────────────────────────────────────────

QTabWidget* SplitView::createTabWidget()
{
    auto* tw = new TabWidgetWithDraggableBar(this);
    tw->setDocumentMode(true);
    tw->installEventFilter(this);
    connect(tw, &QTabWidget::currentChanged, this, &SplitView::onTabWidgetCurrentChanged);

    // Install filter on the tab widget and all its children (editor, close buttons, etc.)
    installFilterOnChildWidgets(tw);

    auto* bar = qobject_cast<DraggableTabBar*>(tw->tabBar());
    if (bar)
    {
        connect(bar, &DraggableTabBar::tabDroppedOutside, this,
                [this, tw](int index, const QString& filePath)
                {
                    auto* editor = tw->widget(index) ? tw->widget(index)->findChild<CodeEditor*>() : nullptr;
                    if (!editor)
                        return;
                    if (totalTabCount() <= 1)
                        return;

                    bool isUntitled = filePath.isEmpty() || filePath == Strings::untitled();
                    if (isUntitled)
                    {
                        QTemporaryFile tmpFile;
                        tmpFile.setAutoRemove(false);
                        if (!tmpFile.open())
                            return;
                        tmpFile.write(editor->text().toUtf8());
                        QString tmpPath = tmpFile.fileName();
                        tmpFile.close();
                        emit tabDetachedToWindow(QString(QChar(1)) + tmpPath);
                    }
                    else
                    {
                        emit tabDetachedToWindow(filePath);
                    }

                    tw->removeTab(index);
                    if (tw->count() == 0)
                    {
                        removePane(tw);
                    }
                });

        connect(bar, &DraggableTabBar::closeTabRequested, this,
                [this, tw](int idx)
                {
                    Q_UNUSED(this)
                    emit tw->tabCloseRequested(idx);
                });

        connect(bar, &DraggableTabBar::closeOtherTabsRequested, this,
                [tw](int keepIdx)
                {
                    for (int i = tw->count() - 1; i >= 0; --i)
                    {
                        if (i != keepIdx)
                            emit tw->tabCloseRequested(i);
                    }
                });

        connect(bar, &DraggableTabBar::closeTabsToRightRequested, this,
                [tw](int pos)
                {
                    for (int i = tw->count() - 1; i > pos; --i)
                        emit tw->tabCloseRequested(i);
                });

        connect(bar, &DraggableTabBar::closeAllTabsRequested, this, [this]() { emit this->closeAllTabsRequested(); });

        connect(bar, &DraggableTabBar::copyPathRequested, this,
                [tw](int idx)
                {
                    auto* w = tw->widget(idx);
                    auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
                    if (editor)
                        QApplication::clipboard()->setText(editor->fileName());
                });

        connect(bar, &DraggableTabBar::copyFileNameRequested, this,
                [tw](int idx)
                {
                    auto* w = tw->widget(idx);
                    auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
                    if (editor)
                        QApplication::clipboard()->setText(QFileInfo(editor->fileName()).fileName());
                });

        connect(bar, &DraggableTabBar::showInFileManagerRequested, this,
                [tw](int idx)
                {
                    auto* w = tw->widget(idx);
                    auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
                    if (editor)
                    {
                        QString path = editor->fileName();
                        if (!path.isEmpty())
                            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).path()));
                    }
                });

        connect(bar, &DraggableTabBar::openInTerminalRequested, this,
                [tw](int idx)
                {
                    auto* w = tw->widget(idx);
                    auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
                    if (!editor)
                        return;
                    QString dirPath = QFileInfo(editor->fileName()).path();
                    QString terminal = qEnvironmentVariable("TERMINAL");
                    if (terminal.isEmpty())
                    {
                        static const QStringList terminals = {QStringLiteral("x-terminal-emulator"),
                                                              QStringLiteral("gnome-terminal"),
                                                              QStringLiteral("konsole"),
                                                              QStringLiteral("xfce4-terminal"),
                                                              QStringLiteral("lxterminal"),
                                                              QStringLiteral("urxvt"),
                                                              QStringLiteral("xterm")};
                        for (const QString& t : terminals)
                        {
                            if (!QStandardPaths::findExecutable(t).isEmpty())
                            {
                                terminal = t;
                                break;
                            }
                        }
                    }
                    if (!terminal.isEmpty())
                        QProcess::startDetached(terminal, QStringList(), dirPath);
                });

        connect(bar, &DraggableTabBar::toggleTabPinnedRequested, this, [this, tw](int idx) { emit this->tabPinToggled(idx, tw); });
    }

    return tw;
}

void SplitView::installFilterOnChildWidgets(QWidget* widget)
{
    if (!widget)
        return;
    widget->installEventFilter(this);
    for (QObject* child : widget->children())
    {
        if (auto* w = qobject_cast<QWidget*>(child))
        {
            if (w->isWindow())
                continue;
            installFilterOnChildWidgets(w);
        }
    }
}

// ── Event Handling ─────────────────────────────────────────────

bool SplitView::eventFilter(QObject* obj, QEvent* event)
{
    switch (event->type())
    {
    case QEvent::DragEnter:
    {
        auto* de = static_cast<QDragEnterEvent*>(event);
        if (de->mimeData()->hasFormat(MIME_TAB))
        {
            m_dragActive = true;
            de->setDropAction(Qt::MoveAction);
            de->accept();
            return true;
        }
        // Also accept file drops for external files
        if (de->mimeData()->hasUrls())
        {
            de->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::DragMove:
    {
        auto* dm = static_cast<QDragMoveEvent*>(event);
        if (dm->mimeData()->hasFormat(MIME_TAB))
        {
            dm->setDropAction(Qt::MoveAction);
            dm->accept();

            QPoint globalPos = static_cast<QWidget*>(obj)->mapToGlobal(dm->position().toPoint());
            QTabBar* bar = tabBarAt(globalPos);
            if (bar)
            {
                if (bar != m_highlightBar)
                {
                    clearDropIndicators();
                    m_highlightBar = bar;
                }
                m_highlightIndex = bar->tabAt(bar->mapFromGlobal(globalPos));
                if (auto* dragBar = qobject_cast<DraggableTabBar*>(bar))
                    dragBar->setDropIndicator(m_highlightIndex);
            }
            else
            {
                if (m_highlightBar)
                {
                    if (auto* oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
                        oldDrag->setDropIndicator(-1);
                    m_highlightBar = nullptr;
                    m_highlightIndex = -1;
                }
                updateDropOverlay(globalPos);
            }
            return true;
        }
        if (dm->mimeData()->hasUrls())
        {
            dm->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::DragLeave:
    {
        if (m_dragActive)
        {
            m_dragActive = false;
            clearDropIndicators();
        }
        break;
    }
    case QEvent::Drop:
    {
        auto* de = static_cast<QDropEvent*>(event);
        if (de->mimeData()->hasFormat(MIME_TAB))
        {
            handleDrop(de);
            m_dragActive = false;
            clearDropIndicators();
            return true;
        }
        if (de->mimeData()->hasUrls())
        {
            for (const QUrl& url : de->mimeData()->urls())
            {
                if (url.isLocalFile())
                {
                    emit externalTabDropped(url.toLocalFile());
                }
            }
            de->acceptProposedAction();
            m_dragActive = false;
            clearDropIndicators();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

void SplitView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(MIME_TAB))
    {
        m_dragActive = true;
        overlay()->setGeometry(rect());
        overlay()->show();
        overlay()->raise();
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
        return;
    }
    QWidget::dragEnterEvent(event);
}

void SplitView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat(MIME_TAB))
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        if (!tabBarAt(mapToGlobal(event->position().toPoint())))
            updateDropOverlay(mapToGlobal(event->position().toPoint()));
        return;
    }
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
        return;
    }
    QWidget::dragMoveEvent(event);
}

void SplitView::dragLeaveEvent(QDragLeaveEvent* event)
{
    m_dragActive = false;
    clearDropIndicators();
    QWidget::dragLeaveEvent(event);
}

void SplitView::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasFormat(MIME_TAB))
    {
        handleDrop(event);
        m_dragActive = false;
        clearDropIndicators();
        return;
    }
    if (event->mimeData()->hasUrls())
    {
        for (const QUrl& url : event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                emit externalTabDropped(url.toLocalFile());
            }
        }
        event->acceptProposedAction();
        m_dragActive = false;
        clearDropIndicators();
        return;
    }
    QWidget::dropEvent(event);
}

void SplitView::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
}

void SplitView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

// ── Drop Handler ───────────────────────────────────────────────

void SplitView::handleDrop(QDropEvent* event)
{
    QByteArray data = event->mimeData()->data(MIME_TAB);
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint64 srcPid = 0;
    quint64 dragId = 0;
    int srcIndex = -1;
    QString filePath;
    stream >> srcPid >> dragId >> srcIndex >> filePath;

    auto* srcTw = s_dragSourceMap.value(dragId).data();
    s_dragSourceMap.remove(dragId);

    if (!srcTw || srcIndex < 0 || srcIndex >= srcTw->count())
    {
        // Cross-process or invalid drop
        if (!filePath.isEmpty())
        {
            emit externalTabDropped(filePath);
            event->setDropAction(Qt::MoveAction);
            event->accept();
            // Acknowledge to source process so it removes the tab
            QString respPath = dragResponsePath(srcPid, dragId);
            QFile respFile(respPath);
            if (respFile.open(QIODevice::WriteOnly))
                respFile.close();
        }
        return;
    }

    handledDragIds().insert(dragId);

    QTabBar* targetBar = tabBarAt(mapToGlobal(event->position().toPoint()));
    if (targetBar)
    {
        auto* targetPane = qobject_cast<QTabWidget*>(targetBar->parent());
        if (!targetPane)
            targetPane = tabWidgetForBar(targetBar);

        if (targetPane)
        {
            QPoint localPt = targetBar->mapFromGlobal(mapToGlobal(event->position().toPoint()));
            int insertAt = targetBar->tabAt(localPt);

            if (targetPane == srcTw)
            {
                if (insertAt >= 0 && insertAt != srcIndex)
                {
                    QWidget* tabW = srcTw->widget(srcIndex);
                    QString text = srcTw->tabText(srcIndex);
                    QIcon icon = srcTw->tabIcon(srcIndex);
                    srcTw->removeTab(srcIndex);
                    int adjust = (insertAt > srcIndex) ? insertAt - 1 : insertAt;
                    srcTw->insertTab(adjust, tabW, icon, text);
                    srcTw->setCurrentWidget(tabW);
                    if (m_onTabMoved)
                        m_onTabMoved(srcTw, adjust);
                }
            }
            else
            {
                if (insertAt < 0)
                    insertAt = targetPane->count();
                QWidget* tabW = srcTw->widget(srcIndex);
                QString text = srcTw->tabText(srcIndex);
                QIcon icon = srcTw->tabIcon(srcIndex);
                QString tooltip = srcTw->tabToolTip(srcIndex);
                srcTw->removeTab(srcIndex);
                int newIdx = targetPane->insertTab(insertAt, tabW, icon, text);
                targetPane->setTabToolTip(newIdx, tooltip);
                targetPane->setCurrentIndex(newIdx);
                tabW->setFocus();
                if (m_onTabMoved)
                    m_onTabMoved(targetPane, newIdx);
            }

            event->setDropAction(Qt::MoveAction);
            event->accept();

            if (srcTw->count() == 0)
            {
                removePane(srcTw);
            }
            return;
        }
    }

    // Not on a tab bar — check for zone-based split
    QTabWidget* targetPane = findPaneAt(mapToGlobal(event->position().toPoint()));
    if (targetPane)
    {
        DropZone zone = calcDropZone(targetPane, mapToGlobal(event->position().toPoint()));

        if (zone == DropZone::Center || m_panes.size() >= MAX_PANES)
        {
            // Add tab to target pane
            if (targetPane == srcTw && srcIndex >= 0)
            {
                // Same pane, already handled by tab bar case above
                event->setDropAction(Qt::MoveAction);
                event->accept();
                return;
            }
            QWidget* tabW = srcTw->widget(srcIndex);
            QString text = srcTw->tabText(srcIndex);
            QIcon icon = srcTw->tabIcon(srcIndex);
            srcTw->removeTab(srcIndex);
            int newIdx = targetPane->addTab(tabW, icon, text);
            targetPane->setCurrentWidget(tabW);
            tabW->setFocus();
            if (m_onTabMoved)
                m_onTabMoved(targetPane, newIdx);
        }
        else
        {
            // Split the pane
            splitPane(targetPane, zone, srcTw, srcIndex);
        }
    }
    else
    {
        // Not on a tab bar or existing pane — don't split if source pane has only one tab
        if (srcTw->count() <= 1)
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        // Create a new pane via split
        if (m_panes.size() < MAX_PANES)
        {
            m_dropping = true;
            // Default to right split
            if (m_panes.size() == 1)
            {
                splitPane(srcTw, DropZone::Right, srcTw, srcIndex);
            }
            else
            {
                // Find the best adjacent pane
                int srcIdx = m_panes.indexOf(srcTw);
                QTabWidget* adjPane = nullptr;
                if (srcIdx + 1 < m_panes.size())
                    adjPane = m_panes[srcIdx + 1];
                else if (srcIdx > 0)
                    adjPane = m_panes[srcIdx - 1];

                if (adjPane)
                    splitPane(adjPane, DropZone::Right, srcTw, srcIndex);
                else
                    splitPane(srcTw, DropZone::Right, srcTw, srcIndex);
            }
            m_dropping = false;
        }
    }

    event->setDropAction(Qt::MoveAction);
    event->accept();

    if (srcTw->count() == 0)
    {
        removePane(srcTw);
    }
}

// ── Pane Lookup ────────────────────────────────────────────────

QTabWidget* SplitView::tabWidgetForBar(QTabBar* bar) const
{
    for (QTabWidget* pane : m_panes)
    {
        if (pane->tabBar() == bar)
            return pane;
    }
    return nullptr;
}

QTabWidget* SplitView::findPaneAt(QPoint screenPos) const
{
    for (QTabWidget* pane : m_panes)
    {
        QRect r(pane->mapToGlobal(QPoint(0, 0)), pane->size());
        if (r.contains(screenPos))
            return pane;
    }
    return nullptr;
}

QTabBar* SplitView::tabBarAt(QPoint screenPos) const
{
    for (QTabWidget* pane : m_panes)
    {
        QTabBar* bar = pane->tabBar();
        if (!bar || !bar->isVisible())
            continue;
        QRect r(bar->mapToGlobal(QPoint(0, 0)), bar->size());
        if (r.contains(screenPos))
            return bar;
    }
    return nullptr;
}

// ── Public API ─────────────────────────────────────────────────

void SplitView::setPaneCallbacks(std::function<void(QTabWidget*)> onAdded, std::function<void(QTabWidget*)> onRemoved,
                                 std::function<void(QTabWidget*, int)> onTabMoved)
{
    m_onPaneAdded = std::move(onAdded);
    m_onPaneRemoved = std::move(onRemoved);
    m_onTabMoved = std::move(onTabMoved);
}

QTabWidget* SplitView::activeTabWidget() const
{
    return m_activeWidget;
}

QTabWidget* SplitView::paneAt(int index) const
{
    if (index >= 0 && index < m_panes.size())
        return m_panes[index];
    return nullptr;
}

QTabWidget* SplitView::addNewPane(QTabWidget* relativeTo, Qt::Orientation orientation, bool after)
{
    if (m_panes.size() >= MAX_PANES)
        return nullptr;

    auto* newPane = createTabWidget();

    QSplitter* parentSplit = parentSplitterFor(relativeTo);
    if (!parentSplit)
        parentSplit = m_splitter;

    if (parentSplit->orientation() == orientation)
    {
        int idx = parentSplit->indexOf(relativeTo);
        if (after)
            idx++;
        parentSplit->insertWidget(idx, newPane);
        distributeSplitter(parentSplit);
    }
    else
    {
        int idx = parentSplit->indexOf(relativeTo);
        auto* subSplit = new QSplitter(orientation);
        subSplit->setChildrenCollapsible(false);
        parentSplit->insertWidget(idx, subSplit);
        if (after)
        {
            subSplit->addWidget(relativeTo);
            subSplit->addWidget(newPane);
        }
        else
        {
            subSplit->addWidget(newPane);
            subSplit->addWidget(relativeTo);
        }
        distributeSplitter(subSplit);
    }

    setupNewPane(newPane);
    return newPane;
}

void SplitView::removePane(QTabWidget* tw)
{
    if ((tw == m_primaryWidget && m_panes.size() == 1) || !m_panes.contains(tw) || m_dropping)
        return;

    QSplitter* parentSplit = parentSplitterFor(tw);

    if (m_onPaneRemoved)
        m_onPaneRemoved(tw);

    if (parentSplit)
    {
        int idx = parentSplit->indexOf(tw);
        // Replace with dummy to remove from layout, then delete it
        auto* dummy = new QWidget();
        dummy->hide();
        parentSplit->replaceWidget(idx, dummy);
        // Collapse dummy to 0 size so remaining panes fill the space
        parentSplit->setStretchFactor(idx, 0);
        QList<int> sz = parentSplit->sizes();
        if (idx < sz.size())
            sz[idx] = 0;
        parentSplit->setSizes(sz);
        dummy->deleteLater();
    }
    m_panes.removeOne(tw);

    if (m_activeWidget == tw)
    {
        m_activeWidget = m_primaryWidget;
    }
    tw->deleteLater();

    // Collapse parent splitter if needed
    if (parentSplit && parentSplit != m_splitter)
        collapseSplitter(parentSplit);

    syncPaneList();

    // If only one pane left, remove the now-unnecessary root splitter orientation change
    if (m_splitter->count() <= 1)
    {
        m_splitter->setOrientation(Qt::Horizontal);
    }
}

bool SplitView::moveTabToPane(int tabIndex, QTabWidget* source, QTabWidget* target, int insertIndex)
{
    if (!source || !target || tabIndex < 0 || tabIndex >= source->count())
        return false;

    QWidget* w = source->widget(tabIndex);
    QString text = source->tabText(tabIndex);
    QIcon icon = source->tabIcon(tabIndex);
    QString tooltip = source->tabToolTip(tabIndex);

    source->removeTab(tabIndex);

    int newIdx;
    if (insertIndex >= 0 && insertIndex <= target->count())
    {
        newIdx = target->insertTab(insertIndex, w, icon, text);
    }
    else
    {
        newIdx = target->addTab(w, icon, text);
    }
    target->setTabToolTip(newIdx, tooltip);
    target->setCurrentWidget(w);
    if (m_onTabMoved)
        m_onTabMoved(target, newIdx);

    if (source->count() == 0)
    {
        removePane(source);
    }
    return true;
}

void SplitView::detachTabToWindow(int tabIndex, QTabWidget* source)
{
    if (!source || tabIndex < 0 || tabIndex >= source->count())
        return;

    auto* editor = qobject_cast<CodeEditor*>(source->widget(tabIndex));
    if (!editor)
        return;

    QString filePath = editor->fileName();
    if (filePath.isEmpty() || filePath == Strings::untitled())
        return;

    emit tabDetachedToWindow(filePath);

    source->removeTab(tabIndex);
    editor->deleteLater();

    if (source->count() == 0)
    {
        removePane(source);
    }
}

void SplitView::onTabWidgetCurrentChanged(int index)
{
    Q_UNUSED(index)
    auto* tw = qobject_cast<QTabWidget*>(sender());
    if (tw)
    {
        m_activeWidget = tw;
        emit activeTabWidgetChanged(tw);
        if (auto* w = tw->currentWidget())
            w->installEventFilter(this);
    }
}

int SplitView::totalTabCount() const
{
    int count = 0;
    for (auto* pane : m_panes)
    {
        count += pane->count();
    }
    return count;
}
