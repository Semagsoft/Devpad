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
#include "codeeditor.h"
#include "appstrings.h"
#include "settingsmanager.h"
#include <QLayout>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QTabBar>
#include <QPainter>
#include <QMenu>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QTemporaryFile>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QSet>
#include <QFile>
#include <QCoreApplication>

static const char *MIME_TAB = "application/x-devpad-tab";

static QString dragResponsePath(qint64 pid, quint64 dragId) {
    return QStringLiteral("/tmp/devpad-drag-resp-%1-%2").arg(pid).arg(dragId);
}

quint64 SplitView::s_dragIdCounter = 0;
QMap<quint64, QPointer<QTabWidget>> SplitView::s_dragSourceMap;

// Tracks drag IDs handled by a same-process handleDrop() call
static QSet<quint64> s_handledDragIds;

static QColor dropHighlightColor() {
    ThemeColors colors = SettingsManager::instance().currentThemeColors();
    return colors.selectionBg;
}

// ── DropOverlay ─────────────────────────────────────────────────

class DropOverlay : public QWidget {
public:
    explicit DropOverlay(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void setHighlightColor(const QColor &c) {
        m_color = QColor(c.red(), c.green(), c.blue(), 128);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), m_color);
    }

private:
    QColor m_color;
};

// ── EditorTabWidget ───────────────────────────────────────────

class EditorTabWidget : public QTabWidget {
public:
    explicit EditorTabWidget(QWidget *parent = nullptr) : QTabWidget(parent) {
        auto *bar = new DraggableTabBar(this);
        QTabWidget::setTabBar(bar);
    }
};

// ── DraggableTabBar ────────────────────────────────────────────

DraggableTabBar::DraggableTabBar(QWidget *parent) : QTabBar(parent) {
    setMovable(true);
    QColor c = dropHighlightColor();
    m_dropColor = QColor(c.red(), c.green(), c.blue(), 180);
}

void DraggableTabBar::mousePressEvent(QMouseEvent *event) {
    m_dragTabIndex = -1;
    if (event->button() == Qt::LeftButton) {
        int idx = tabAt(event->pos());
        if (idx >= 0) {
            m_dragTabIndex = idx;
            m_dragStartPos = event->pos();
            m_dragging = false;
        }
    }
    QTabBar::mousePressEvent(event);
}

void DraggableTabBar::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragTabIndex < 0 || !(event->buttons() & Qt::LeftButton)) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if (!m_dragging && (event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
        // If cursor is still within the tab bar, let QTabBar handle native reorder
        if (rect().contains(event->pos())) {
            QTabBar::mouseMoveEvent(event);
            return;
        }

        // Cursor left the tab bar — start QDrag for cross-pane / external move
        m_dragging = true;
        emit tabDragStarted(m_dragTabIndex);

        // Reset QTabBar's internal movable state so it doesn't hang mid-drag
        setMovable(false);
        setMovable(true);

        auto *tabWidget = qobject_cast<QTabWidget*>(parent());
        quint64 dragId = ++SplitView::s_dragIdCounter;
        SplitView::s_dragSourceMap[dragId] = tabWidget;

        auto *expectedWidget = tabWidget->widget(m_dragTabIndex);
        auto *ce = qobject_cast<CodeEditor*>(expectedWidget);
        QString filePath = ce ? ce->fileName() : QString();

        // Remove any stale response file from a previous run
        QFile::remove(dragResponsePath(QCoreApplication::applicationPid(), dragId));

        QDrag *drag = new QDrag(this);
        auto *mimeData = new QMimeData();

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        qint64 srcPid = QCoreApplication::applicationPid();
        stream << srcPid << dragId << m_dragTabIndex << filePath;
        mimeData->setData(MIME_TAB, data);
        if (ce && !filePath.isEmpty()) {
            QList<QUrl> urls;
            urls << QUrl::fromLocalFile(filePath);
            mimeData->setUrls(urls);
        }
        drag->setMimeData(mimeData);

        QRect r = tabRect(m_dragTabIndex);
        if (r.isValid()) {
            QPixmap pixmap(r.size() * devicePixelRatioF());
            pixmap.setDevicePixelRatio(devicePixelRatioF());
            pixmap.fill(Qt::transparent);
            QPainter p(&pixmap);
            render(&p, QPoint(), QRegion(r));
            p.end();
            drag->setPixmap(pixmap);
            drag->setHotSpot(QPoint(r.width() / 2, r.height() / 2));
        }

        Qt::DropAction result = drag->exec(Qt::MoveAction);

        SplitView::s_dragSourceMap.remove(dragId);

        auto removeTabFromSource = [&]() {
            if (tabWidget && m_dragTabIndex >= 0 && m_dragTabIndex < tabWidget->count()) {
                if (tabWidget->widget(m_dragTabIndex) == expectedWidget) {
                    tabWidget->removeTab(m_dragTabIndex);
                    if (tabWidget->count() == 0) {
                        for (QWidget *p = tabWidget->parentWidget(); p; p = p->parentWidget()) {
                            if (auto *sv = qobject_cast<SplitView*>(p)) {
                                sv->removePane(tabWidget);
                                break;
                            }
                        }
                    }
                }
            }
        };

        if (result != Qt::IgnoreAction && !s_handledDragIds.contains(dragId)) {
            // Cross-process drop was accepted by another window — remove tab from source
            s_handledDragIds.remove(dragId);
            removeTabFromSource();
        } else if (result == Qt::IgnoreAction) {
            // Check if a cross-process target acknowledged the drop via response file
            QString respPath = dragResponsePath(QCoreApplication::applicationPid(), dragId);
            bool crossProcessAccepted = QFile::exists(respPath);
            if (crossProcessAccepted) {
                QFile::remove(respPath);
                removeTabFromSource();
            } else if (tabWidget) {
                auto *ce = qobject_cast<CodeEditor*>(tabWidget->widget(m_dragTabIndex));
                QString filePath = ce ? ce->fileName() : QString();
                emit tabDroppedOutside(m_dragTabIndex, filePath);
            }
        }

        s_handledDragIds.remove(dragId);
        m_dragTabIndex = -1;
        m_dragging = false;
        return;
    }

    QTabBar::mouseMoveEvent(event);
}

void DraggableTabBar::setDropIndicator(int index) {
    if (m_dropIndicatorIndex != index) {
        m_dropIndicatorIndex = index;
        update();
    }
}

void DraggableTabBar::setDropIndicatorColor(const QColor &color) {
    m_dropColor = QColor(color.red(), color.green(), color.blue(), 180);
    if (m_dropIndicatorIndex >= 0) update();
}

void DraggableTabBar::paintEvent(QPaintEvent *event) {
    QTabBar::paintEvent(event);
    if (m_dropIndicatorIndex >= 0 && m_dropIndicatorIndex < count()) {
        QRect r = tabRect(m_dropIndicatorIndex);
        if (r.isValid()) {
            QPainter p(this);
            p.setPen(Qt::NoPen);
            p.setBrush(m_dropColor);
            p.drawRect(r.adjusted(0, 0, 0, 0));
        }
    }
}

void DraggableTabBar::contextMenuEvent(QContextMenuEvent *event) {
    int idx = tabAt(event->pos());
    if (idx < 0) return;

    auto *tw = qobject_cast<QTabWidget*>(parent());
    auto *editor = tw ? qobject_cast<CodeEditor*>(tw->widget(idx)) : nullptr;
    if (!editor) return;

    bool isUntitled = editor->fileName().isEmpty() || editor->fileName() == Strings::untitled();
    int tabCount = count();
    bool readOnly = editor->isReadOnly();

    QMenu menu(this);

    QAction *closeAct = menu.addAction(QIcon(":/icons/File/close.svg"), tr("Close"));
    closeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    closeAct->setEnabled(!readOnly);

    QAction *closeOthersAct = menu.addAction(tr("Close Others"));
    closeOthersAct->setEnabled(tabCount > 1 && !readOnly);

    QAction *closeRightAct = menu.addAction(tr("Close to the Right"));
    closeRightAct->setEnabled(idx < tabCount - 1 && !readOnly);

    QAction *closeAllAct = menu.addAction(QIcon(":/icons/File/closeall.svg"), tr("Close All"));
    closeAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_W));
    closeAllAct->setEnabled(tabCount > 0 && !readOnly);

    menu.addSeparator();

    QAction *pinAct = menu.addAction(QStringLiteral("\U0001F4CC ") + tr("Pin Tab"));
    pinAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P));

    menu.addSeparator();

    QAction *copyPathAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy Path"));
    copyPathAct->setEnabled(!isUntitled);

    QAction *copyNameAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy File Name"));
    copyNameAct->setEnabled(!isUntitled);

    menu.addSeparator();

    QAction *showInFmAct = menu.addAction(QIcon(":/icons/Common/openinfolder.svg"), tr("Show in File Manager"));
    showInFmAct->setEnabled(!isUntitled);

    QAction *openTermAct = menu.addAction(QIcon(":/icons/Common/openinterminal.svg"), tr("Open in Terminal"));
    openTermAct->setEnabled(!isUntitled);

    QAction *chosen = menu.exec(event->globalPos());
    if (!chosen) return;

    if (chosen == closeAct)
        emit closeTabRequested(idx);
    else if (chosen == closeOthersAct)
        emit closeOtherTabsRequested(idx);
    else if (chosen == closeRightAct)
        emit closeTabsToRightRequested(idx);
    else if (chosen == closeAllAct)
        emit closeAllTabsRequested();
    else if (chosen == pinAct)
        emit toggleTabPinnedRequested(idx);
    else if (chosen == copyPathAct)
        emit copyPathRequested(idx);
    else if (chosen == copyNameAct)
        emit copyFileNameRequested(idx);
    else if (chosen == showInFmAct)
        emit showInFileManagerRequested(idx);
    else if (chosen == openTermAct)
        emit openInTerminalRequested(idx);
}

// ── SplitView ──────────────────────────────────────────────────

SplitView::SplitView(QWidget *parent) : QWidget(parent) {
    setAcceptDrops(true);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->installEventFilter(this);

    m_dropOverlay = new DropOverlay(this);
    QColor c = dropHighlightColor();
    static_cast<DropOverlay*>(m_dropOverlay)->setHighlightColor(c);
    m_dropOverlay->hide();

    m_primaryWidget = createTabWidget();
    m_splitter->addWidget(m_primaryWidget);
    m_panes.append(m_primaryWidget);
    m_activeWidget = m_primaryWidget;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
}

SplitView::~SplitView() = default;

QTabWidget* SplitView::createTabWidget() {
    auto *tw = new EditorTabWidget(this);
    tw->setDocumentMode(true);
    tw->installEventFilter(this);
    connect(tw, &QTabWidget::currentChanged, this, &SplitView::onTabWidgetCurrentChanged);

    // Install filter on the tab bar and all its children (close buttons etc.)
    installFilterOnChildWidgets(tw->tabBar());

    auto *bar = qobject_cast<DraggableTabBar*>(tw->tabBar());
    if (bar) {
        connect(bar, &DraggableTabBar::tabDroppedOutside, this, [this, tw](int index, const QString &filePath) {
            auto *editor = qobject_cast<CodeEditor*>(tw->widget(index));
            if (!editor) return;
            if (totalTabCount() <= 1) return;

            bool isUntitled = filePath.isEmpty() || filePath == Strings::untitled();
            if (isUntitled) {
                QTemporaryFile tmpFile;
                tmpFile.setAutoRemove(false);
                if (!tmpFile.open()) return;
                tmpFile.write(editor->text().toUtf8());
                QString tmpPath = tmpFile.fileName();
                tmpFile.close();
                emit tabDetachedToWindow(QString(QChar(1)) + tmpPath);
            } else {
                emit tabDetachedToWindow(filePath);
            }

            tw->removeTab(index);
            if (tw->count() == 0) {
                removePane(tw);
            }
        });

        connect(bar, &DraggableTabBar::closeTabRequested, this, [this, tw](int idx) {
            Q_UNUSED(this)
            emit tw->tabCloseRequested(idx);
        });

        connect(bar, &DraggableTabBar::closeOtherTabsRequested, this, [tw](int keepIdx) {
            for (int i = tw->count() - 1; i >= 0; --i) {
                if (i != keepIdx)
                    emit tw->tabCloseRequested(i);
            }
        });

        connect(bar, &DraggableTabBar::closeTabsToRightRequested, this, [tw](int pos) {
            for (int i = tw->count() - 1; i > pos; --i)
                emit tw->tabCloseRequested(i);
        });

        connect(bar, &DraggableTabBar::closeAllTabsRequested, this, [this]() {
            emit this->closeAllTabsRequested();
        });

        connect(bar, &DraggableTabBar::copyPathRequested, this, [tw](int idx) {
            auto *editor = qobject_cast<CodeEditor*>(tw->widget(idx));
            if (editor)
                QApplication::clipboard()->setText(editor->fileName());
        });

        connect(bar, &DraggableTabBar::copyFileNameRequested, this, [tw](int idx) {
            auto *editor = qobject_cast<CodeEditor*>(tw->widget(idx));
            if (editor)
                QApplication::clipboard()->setText(QFileInfo(editor->fileName()).fileName());
        });

        connect(bar, &DraggableTabBar::showInFileManagerRequested, this, [tw](int idx) {
            auto *editor = qobject_cast<CodeEditor*>(tw->widget(idx));
            if (editor) {
                QString path = editor->fileName();
                if (!path.isEmpty())
                    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).path()));
            }
        });

        connect(bar, &DraggableTabBar::openInTerminalRequested, this, [tw](int idx) {
            auto *editor = qobject_cast<CodeEditor*>(tw->widget(idx));
            if (!editor) return;
            QString dirPath = QFileInfo(editor->fileName()).path();
            QString terminal = qEnvironmentVariable("TERMINAL");
            if (terminal.isEmpty()) {
                static const QStringList terminals = {
                    QStringLiteral("x-terminal-emulator"), QStringLiteral("gnome-terminal"),
                    QStringLiteral("konsole"), QStringLiteral("xfce4-terminal"),
                    QStringLiteral("lxterminal"), QStringLiteral("urxvt"), QStringLiteral("xterm")
                };
                for (const QString &t : terminals) {
                    if (!QStandardPaths::findExecutable(t).isEmpty()) {
                        terminal = t;
                        break;
                    }
                }
            }
            if (!terminal.isEmpty())
                QProcess::startDetached(terminal, QStringList(), dirPath);
        });

        connect(bar, &DraggableTabBar::toggleTabPinnedRequested, this, [this, tw](int idx) {
            emit this->tabPinToggled(idx, tw);
        });
    }

    return tw;
}

void SplitView::installFilterOnChildWidgets(QWidget *widget) {
    if (!widget) return;
    widget->installEventFilter(this);
    for (QObject *child : widget->children()) {
        if (auto *w = qobject_cast<QWidget*>(child)) {
            if (w->isWindow()) continue;
            installFilterOnChildWidgets(w);
        }
    }
}

bool SplitView::eventFilter(QObject *obj, QEvent *event) {
    switch (event->type()) {
    case QEvent::DragEnter: {
        m_highlightBar = nullptr;
        m_highlightIndex = -1;
        auto *de = static_cast<QDragEnterEvent*>(event);
        if (de->mimeData()->hasFormat(MIME_TAB)) {
            m_dragActive = true;
            m_dropOverlay->setGeometry(rect());
            m_dropOverlay->raise();
            m_dropOverlay->show();
            de->setDropAction(Qt::MoveAction);
            de->accept();
            return true;
        }
        break;
    }
    case QEvent::DragMove: {
        auto *dm = static_cast<QDragMoveEvent*>(event);
        if (dm->mimeData()->hasFormat(MIME_TAB)) {
            // Update highlight
            QWidget *under = QApplication::widgetAt(QCursor::pos());
            QTabBar *bar = nullptr;
            while (under && under != this) {
                bar = qobject_cast<QTabBar*>(under);
                if (bar) break;
                under = under->parentWidget();
            }

            if (bar != m_highlightBar) {
                if (m_highlightBar) {
                    if (auto *oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
                        oldDrag->setDropIndicator(-1);
                }
                m_highlightBar = bar;
            }
            if (bar) {
                m_highlightIndex = bar->tabAt(bar->mapFromGlobal(QCursor::pos()));
                if (auto *dragBar = qobject_cast<DraggableTabBar*>(bar))
                    dragBar->setDropIndicator(m_highlightIndex);
            } else {
                m_highlightIndex = -1;
            }

            dm->setDropAction(Qt::MoveAction);
            dm->accept();
            return true;
        }
        break;
    }
    case QEvent::DragLeave: {
        if (m_dragActive) {
            m_dragActive = false;
            m_dropOverlay->hide();
            if (m_highlightBar) {
                if (auto *oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
                    oldDrag->setDropIndicator(-1);
                m_highlightBar = nullptr;
            }
            m_highlightIndex = -1;
        }
        break;
    }
    case QEvent::Drop: {
        auto *de = static_cast<QDropEvent*>(event);
        if (de->mimeData()->hasFormat(MIME_TAB)) {
            handleDrop(de);
            m_dragActive = false;
            m_dropOverlay->hide();
            if (m_highlightBar) {
                if (auto *oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
                    oldDrag->setDropIndicator(-1);
                m_highlightBar = nullptr;
            }
            m_highlightIndex = -1;
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

void SplitView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat(MIME_TAB)) {
        m_dragActive = true;
        m_dropOverlay->setGeometry(rect());
        m_dropOverlay->raise();
        m_dropOverlay->show();
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    QWidget::dragEnterEvent(event);
}

void SplitView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat(MIME_TAB)) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    QWidget::dragMoveEvent(event);
}

void SplitView::dragLeaveEvent(QDragLeaveEvent *event) {
    m_dragActive = false;
    m_dropOverlay->hide();
    if (m_highlightBar) {
        if (auto *oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
            oldDrag->setDropIndicator(-1);
        m_highlightBar = nullptr;
    }
    m_highlightIndex = -1;
    QWidget::dragLeaveEvent(event);
}

void SplitView::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat(MIME_TAB)) {
        handleDrop(event);
        m_dragActive = false;
        m_dropOverlay->hide();
        if (m_highlightBar) {
            if (auto *oldDrag = qobject_cast<DraggableTabBar*>(m_highlightBar))
                oldDrag->setDropIndicator(-1);
            m_highlightBar = nullptr;
        }
        m_highlightIndex = -1;
        return;
    }
    QWidget::dropEvent(event);
}

void SplitView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
}

void SplitView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_dropOverlay->isVisible()) {
        m_dropOverlay->setGeometry(rect());
    }
}

void SplitView::handleDrop(QDropEvent *event) {
    QByteArray data = event->mimeData()->data(MIME_TAB);
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint64 srcPid = 0;
    quint64 dragId = 0;
    int srcIndex = -1;
    QString filePath;
    stream >> srcPid >> dragId >> srcIndex >> filePath;

    auto *srcTw = s_dragSourceMap.value(dragId).data();
    s_dragSourceMap.remove(dragId);

    if (!srcTw || srcIndex < 0 || srcIndex >= srcTw->count()) {
        // Cross-process or invalid drop
        if (!filePath.isEmpty()) {
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

    s_handledDragIds.insert(dragId);

    // Find the actual widget under the cursor
    QWidget *under = QApplication::widgetAt(QCursor::pos());

    // Walk up to find a tab bar
    QTabBar *targetBar = nullptr;
    QWidget *w = under;
    while (w && w != this) {
        targetBar = qobject_cast<QTabBar*>(w);
        if (targetBar) break;
        w = w->parentWidget();
    }

    if (targetBar) {
        auto *targetPane = qobject_cast<QTabWidget*>(targetBar->parent());
        if (!targetPane) targetPane = tabWidgetForBar(targetBar);

        if (targetPane) {
            QPoint localPt = targetBar->mapFromGlobal(QCursor::pos());
            int insertAt = targetBar->tabAt(localPt);

            if (targetPane == srcTw) {
                if (insertAt >= 0 && insertAt != srcIndex) {
                    QWidget *tabW = srcTw->widget(srcIndex);
                    QString text = srcTw->tabText(srcIndex);
                    QIcon icon = srcTw->tabIcon(srcIndex);
                    srcTw->removeTab(srcIndex);
                    int adjust = (insertAt > srcIndex) ? insertAt - 1 : insertAt;
                    srcTw->insertTab(adjust, tabW, icon, text);
                    srcTw->setCurrentWidget(tabW);
                    if (m_onTabMoved) m_onTabMoved(srcTw, adjust);
                }
            } else {
                if (insertAt < 0) insertAt = targetPane->count();
                QWidget *tabW = srcTw->widget(srcIndex);
                QString text = srcTw->tabText(srcIndex);
                QIcon icon = srcTw->tabIcon(srcIndex);
                QString tooltip = srcTw->tabToolTip(srcIndex);
                srcTw->removeTab(srcIndex);
                int newIdx = targetPane->insertTab(insertAt, tabW, icon, text);
                targetPane->setTabToolTip(newIdx, tooltip);
                targetPane->setCurrentIndex(newIdx);
                tabW->setFocus();
                if (m_onTabMoved) m_onTabMoved(targetPane, newIdx);
            }

            event->setDropAction(Qt::MoveAction);
            event->accept();

            if (srcTw->count() == 0) {
                removePane(srcTw);
            }
            return;
        }
    }

    // Not on a tab bar — check if over an existing pane's editor area
    QTabWidget *targetPane = findPaneAt(QCursor::pos());
    if (targetPane && targetPane != srcTw) {
        QWidget *tabW = srcTw->widget(srcIndex);
        QString text = srcTw->tabText(srcIndex);
        QIcon icon = srcTw->tabIcon(srcIndex);
        srcTw->removeTab(srcIndex);
        int newIdx = targetPane->addTab(tabW, icon, text);
        targetPane->setCurrentWidget(tabW);
        tabW->setFocus();
        if (m_onTabMoved) m_onTabMoved(targetPane, newIdx);
    } else {
        // Not on a tab bar or existing pane — don't split if source pane has only one tab
        if (srcTw->count() <= 1) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        // Create a new pane
        m_dropping = true;
        QTabWidget *newPane = addNewPane(Qt::Horizontal, m_panes.indexOf(srcTw));
        if (!newPane) return;

        QWidget *tabW = srcTw->widget(srcIndex);
        QString text = srcTw->tabText(srcIndex);
        QIcon icon = srcTw->tabIcon(srcIndex);
        srcTw->removeTab(srcIndex);
        int newIdx = newPane->addTab(tabW, icon, text);
        newPane->setCurrentWidget(tabW);
        tabW->setFocus();
        if (m_onTabMoved) m_onTabMoved(newPane, newIdx);
        m_dropping = false;
    }

    event->setDropAction(Qt::MoveAction);
    event->accept();

    if (srcTw->count() == 0) {
        removePane(srcTw);
    }
}

QTabWidget* SplitView::tabWidgetForBar(QTabBar *bar) const {
    for (QTabWidget *pane : m_panes) {
        if (pane->tabBar() == bar) return pane;
    }
    return nullptr;
}

QTabWidget* SplitView::findPaneAt(QPoint screenPos) const {
    QWidget *w = QApplication::widgetAt(screenPos);
    while (w) {
        auto *tw = qobject_cast<QTabWidget*>(w);
        if (tw && m_panes.contains(tw)) return tw;
        w = w->parentWidget();
        if (!w || w == this || w == m_splitter) break;
    }
    return nullptr;
}

void SplitView::setPaneCallbacks(std::function<void(QTabWidget*)> onAdded,
                                 std::function<void(QTabWidget*)> onRemoved,
                                 std::function<void(QTabWidget*, int)> onTabMoved) {
    m_onPaneAdded = std::move(onAdded);
    m_onPaneRemoved = std::move(onRemoved);
    m_onTabMoved = std::move(onTabMoved);
}

QTabWidget* SplitView::activeTabWidget() const {
    return m_activeWidget;
}

QTabWidget* SplitView::paneAt(int index) const {
    if (index >= 0 && index < m_panes.size()) return m_panes[index];
    return nullptr;
}

QTabWidget* SplitView::addNewPane(Qt::Orientation orientation, int index) {
    auto *newPane = createTabWidget();

    m_splitter->setOrientation(orientation);
    if (index >= 0 && index < m_panes.size()) {
        int splitterIdx = m_splitter->indexOf(m_panes[index]);
        m_splitter->insertWidget(splitterIdx + 1, newPane);
        m_panes.insert(index + 1, newPane);
    } else {
        m_splitter->addWidget(newPane);
        m_panes.append(newPane);
    }

    if (m_onPaneAdded) m_onPaneAdded(newPane);

    return newPane;
}

void SplitView::removePane(QTabWidget *tw) {
    if ((tw == m_primaryWidget && m_panes.size() == 1) || !m_panes.contains(tw) || m_dropping) return;

    if (m_onPaneRemoved) m_onPaneRemoved(tw);

    int idx = m_splitter->indexOf(tw);
    if (idx >= 0) {
        m_splitter->widget(idx)->hide();
    }
    m_panes.removeOne(tw);

    if (m_activeWidget == tw) {
        m_activeWidget = m_primaryWidget;
    }
    tw->deleteLater();

    if (m_splitter->count() <= 1) {
        m_splitter->setOrientation(Qt::Horizontal);
    }
}

bool SplitView::moveTabToPane(int tabIndex, QTabWidget *source, QTabWidget *target, int insertIndex) {
    if (!source || !target || tabIndex < 0 || tabIndex >= source->count()) return false;

    QWidget *w = source->widget(tabIndex);
    QString text = source->tabText(tabIndex);
    QIcon icon = source->tabIcon(tabIndex);
    QString tooltip = source->tabToolTip(tabIndex);

    source->removeTab(tabIndex);

    int newIdx;
    if (insertIndex >= 0 && insertIndex <= target->count()) {
        newIdx = target->insertTab(insertIndex, w, icon, text);
    } else {
        newIdx = target->addTab(w, icon, text);
    }
    target->setTabToolTip(newIdx, tooltip);
    target->setCurrentWidget(w);
    if (m_onTabMoved) m_onTabMoved(target, newIdx);

    if (source->count() == 0) {
        removePane(source);
    }
    return true;
}

void SplitView::detachTabToWindow(int tabIndex, QTabWidget *source) {
    if (!source || tabIndex < 0 || tabIndex >= source->count()) return;

    auto *editor = qobject_cast<CodeEditor*>(source->widget(tabIndex));
    if (!editor) return;

    QString filePath = editor->fileName();
    if (filePath.isEmpty() || filePath == Strings::untitled()) return;

    emit tabDetachedToWindow(filePath);

    source->removeTab(tabIndex);
    editor->deleteLater();

    if (source->count() == 0) {
        removePane(source);
    }
}

void SplitView::onTabWidgetCurrentChanged(int index) {
    Q_UNUSED(index)
    auto *tw = qobject_cast<QTabWidget*>(sender());
    if (tw) {
        m_activeWidget = tw;
        emit activeTabWidgetChanged(tw);
    }
}

int SplitView::totalTabCount() const {
    int count = 0;
    for (auto *pane : m_panes) {
        count += pane->count();
    }
    return count;
}
