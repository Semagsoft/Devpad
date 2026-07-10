#include "draggabletabbar.h"
#include "codeeditor.h"
#include "appstrings.h"
#include "settingsmanager.h"
#include "splitview.h"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTemporaryFile>

const char* MIME_TAB = "application/x-devpad-tab";

QString dragResponsePath(qint64 pid, quint64 dragId)
{
    return QDir::tempPath() + QStringLiteral("/devpad-drag-resp-%1-%2").arg(pid).arg(dragId);
}

QColor dropHighlightColor()
{
    ThemeColors colors = SettingsManager::instance().currentThemeColors();
    return colors.selectionBg;
}

// ── DraggableTabBar ────────────────────────────────────────────

DraggableTabBar::DraggableTabBar(QWidget* parent)
    : QTabBar(parent)
{
    setMovable(true);
    QColor c = dropHighlightColor();
    m_dropColor = QColor(c.red(), c.green(), c.blue(), 180);
}

void DraggableTabBar::mousePressEvent(QMouseEvent* event)
{
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

void DraggableTabBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragTabIndex < 0 || !(event->buttons() & Qt::LeftButton)) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if (!m_dragging && (event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
        if (rect().contains(event->pos())) {
            QTabBar::mouseMoveEvent(event);
            return;
        }

        m_dragging = true;
        emit tabDragStarted(m_dragTabIndex);

        setMovable(false);
        setMovable(true);

        auto* tabWidget = qobject_cast<QTabWidget*>(parent());
        quint64 dragId = SplitView::nextDragId();
        SplitView::registerDragSource(dragId, tabWidget);

        auto* expectedWidget = tabWidget->widget(m_dragTabIndex);
        auto* ce = expectedWidget ? expectedWidget->findChild<CodeEditor*>() : nullptr;
        QString filePath = ce ? ce->fileName() : QString();

        QFile::remove(dragResponsePath(QCoreApplication::applicationPid(), dragId));

        QDrag* drag = new QDrag(this);
        auto* mimeData = new QMimeData();

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

        SplitView::removeDragSource(dragId);

        auto removeTabFromSource = [&]() {
            if (tabWidget && m_dragTabIndex >= 0 && m_dragTabIndex < tabWidget->count()) {
                if (tabWidget->widget(m_dragTabIndex) == expectedWidget) {
                    tabWidget->removeTab(m_dragTabIndex);
                    if (tabWidget->count() == 0) {
                        for (QWidget* p = tabWidget->parentWidget(); p; p = p->parentWidget()) {
                            if (auto* sv = qobject_cast<SplitView*>(p)) {
                                sv->removePane(tabWidget);
                                break;
                            }
                        }
                    }
                }
            }
        };

        if (result != Qt::IgnoreAction && !handledDragIds().contains(dragId)) {
            handledDragIds().remove(dragId);
            removeTabFromSource();
        } else if (result == Qt::IgnoreAction) {
            QString respPath = dragResponsePath(QCoreApplication::applicationPid(), dragId);
            bool crossProcessAccepted = QFile::exists(respPath);
            if (crossProcessAccepted) {
                QFile::remove(respPath);
                removeTabFromSource();
            } else if (tabWidget) {
                auto* w = tabWidget->widget(m_dragTabIndex);
                auto* ce = w ? w->findChild<CodeEditor*>() : nullptr;
                QString filePath = ce ? ce->fileName() : QString();
                emit tabDroppedOutside(m_dragTabIndex, filePath);
            }
        }

        handledDragIds().remove(dragId);
        m_dragTabIndex = -1;
        m_dragging = false;
        return;
    }

    QTabBar::mouseMoveEvent(event);
}

void DraggableTabBar::setDropIndicator(int index)
{
    if (m_dropIndicatorIndex != index) {
        m_dropIndicatorIndex = index;
        update();
    }
}

void DraggableTabBar::setDropIndicatorColor(const QColor& color)
{
    m_dropColor = QColor(color.red(), color.green(), color.blue(), 180);
    if (m_dropIndicatorIndex >= 0)
        update();
}

void DraggableTabBar::paintEvent(QPaintEvent* event)
{
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

void DraggableTabBar::contextMenuEvent(QContextMenuEvent* event)
{
    int idx = tabAt(event->pos());
    if (idx < 0)
        return;

    auto* tw = qobject_cast<QTabWidget*>(parent());
    auto* w = tw ? tw->widget(idx) : nullptr;
    auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
    if (!editor)
        return;

    bool isUntitled = editor->fileName().isEmpty() || editor->fileName() == Strings::untitled();
    int tabCount = count();
    bool readOnly = editor->isReadOnly();

    QMenu menu(this);

    QAction* closeAct = menu.addAction(QIcon(":/icons/File/close.svg"), tr("Close"));
    closeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    closeAct->setEnabled(!readOnly);

    QAction* closeOthersAct = menu.addAction(tr("Close Others"));
    closeOthersAct->setEnabled(tabCount > 1 && !readOnly);

    QAction* closeRightAct = menu.addAction(tr("Close to the Right"));
    closeRightAct->setEnabled(idx < tabCount - 1 && !readOnly);

    QAction* closeAllAct = menu.addAction(QIcon(":/icons/File/closeall.svg"), tr("Close All"));
    closeAllAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_W));
    closeAllAct->setEnabled(tabCount > 0 && !readOnly);

    menu.addSeparator();

    QAction* pinAct = menu.addAction(QStringLiteral("\U0001F4CC ") + tr("Pin Tab"));
    pinAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P));

    menu.addSeparator();

    QAction* copyPathAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy Path"));
    copyPathAct->setEnabled(!isUntitled);

    QAction* copyNameAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy File Name"));
    copyNameAct->setEnabled(!isUntitled);

    menu.addSeparator();

    QAction* showInFmAct = menu.addAction(QIcon(":/icons/Common/openinfolder.svg"), tr("Show in File Manager"));
    showInFmAct->setEnabled(!isUntitled);

    QAction* openTermAct = menu.addAction(QIcon(":/icons/Common/openinterminal.svg"), tr("Open in Terminal"));
    openTermAct->setEnabled(!isUntitled);

    QAction* chosen = menu.exec(event->globalPos());
    if (!chosen)
        return;

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
