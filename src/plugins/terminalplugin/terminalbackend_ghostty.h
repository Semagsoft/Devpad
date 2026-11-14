/*
 * Devpad - Terminal backend backed by libghostty
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

#ifndef TERMINALBACKEND_GHOSTTY_H
#define TERMINALBACKEND_GHOSTTY_H

#include "terminalbackend.h"
#include "theme.h"

#include <QEvent>
#include <QClipboard>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMutex>
#include <QPixmap>
#include <QScrollBar>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <QContextMenuEvent>

class QShowEvent;
class QPaintEvent;
class QResizeEvent;

#include <ghostty/vt.h>
#include <ghostty/vt/mouse.h>

/**
 * TerminalBackend backed by libghostty-vt (ghostty.org).
 *
 * libghostty owns the VT parser + terminal state; this backend owns the
 * PTY (shell process) and the rendering surface. The backend renders into a
 * persistent frame pixmap; paintEvent() only blits the cached frame.
 */
class TerminalBackendGhostty : public TerminalBackend
{
    Q_OBJECT

public:
    TerminalBackendGhostty(QWidget* parent = nullptr);
    ~TerminalBackendGhostty() override;

    void start() override;
    void stop() override;
    void setWorkingDirectory(const QString& path) override;
    void setTerminalFont(const QFont& font) override;
    void applyTheme(const QString& schemeName) override;
    void copyClipboard() override;
    void pasteClipboard() override;
    void pasteFromClipboard(QClipboard::Mode mode);
    void sendText(const QString& text) override;
    bool isRunning() const override;
    QWidget* terminalWidget() const override;

    void keyPressEvent(QKeyEvent* event) override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void handlePtyData(const QByteArray& data);
    void handlePtyEof();
    void handleBell();

private:
    // -- libghostty C API callbacks ----------------------------------------
    static void writePtyCallback(GhosttyTerminal terminal, void* userdata, const uint8_t* data, size_t len);
    static void bellCallback(GhosttyTerminal terminal, void* userdata);
    static bool ghosttyMimeReader(void* userdata, GhosttyString mime, GhosttyWriter writer);

    // -- PTY + IO ----------------------------------------------------------
    bool startPty();
    void stopPty();
    void startIoThread();
    void stopIoThread();

    // -- Rendering --------------------------------------------------------
    void renderFrame();
    void resizeTerminal();
    void updateGridSize(const QSize& size);

    // -- Input -------------------------------------------------------------
    void writeToMaster(const uint8_t* data, size_t len);
    void positionGestureEvents(const QPoint& pos);
    uint64_t nowNs() const;
    void updateScrollbar();
    void updateAutoScrollTimer();
    void autoScrollTick();

    // -- Mouse reporting (xterm-style SGR/X10 escape sequences) -------------
    void updateMouseEncoderSize();
    bool isMouseTracking() const;
    GhosttyMouseButton qtButtonToGhostty(Qt::MouseButton button) const;
    GhosttyMods qtModsToGhostty(Qt::KeyboardModifiers mods) const;
    void reportMouseEvent(GhosttyMouseAction action, GhosttyMouseButton button, const QPointF& pos, Qt::KeyboardModifiers mods);
    void updatePointerShape();

    // -- URL detection + Ctrl-click ------------------------------------------
    void rebuildUrlCache();
    QString urlAt(const QPointF& pos, int* hoverRow = nullptr, int* startCol = nullptr, int* endCol = nullptr);
    struct UrlSpan
    {
        int startCol;
        int endCol;
        QString url;
    };
    QVector<QList<UrlSpan>> m_urlSpans; // per visible row
    bool m_urlCacheValid = false;
    QString m_hoverUrl; // URL currently hovered under Ctrl
    int m_hoverRow = -1;
    int m_hoverStartCol = 0;
    int m_hoverEndCol = -1;

    // -- Backing state ----------------------------------------------------
    GhosttyTerminal m_terminal = nullptr;
    GhosttyKeyEncoder m_keyEncoder = nullptr;

    int m_masterFd = -1;
    pid_t m_pid = -1;
    QThread* m_ioThread = nullptr;
    bool m_stopRequested = false;

    QString m_workingDir;
    QFont m_cachedFont;
    bool m_running = false;

    // Selection gesture (mouse drag) and the reusable press/drag/release events.
    GhosttySelectionGesture m_selectionGesture = nullptr;
    GhosttySelectionGestureEvent m_selectPressEvent = nullptr;
    GhosttySelectionGestureEvent m_selectDragEvent = nullptr;
    GhosttySelectionGestureEvent m_selectReleaseEvent = nullptr;
    bool m_selecting = false;

    // Theme selection colors (fill = translucent overlay on selected cells).
    QColor m_selectionBg;
    QColor m_selectionFg;

    // Shell exit status reported through finished().
    int m_exitCode = -1;
    int m_exitStatus = -1;

    // Mouse reporting state.
    GhosttyMouseEncoder m_mouseEncoder = nullptr;
    GhosttyMouseEvent m_mouseEvent = nullptr;
    Qt::MouseButtons m_mouseButtonsDown;
    int m_lastPointerShape = -1;
    QClipboard::Mode m_pasteMode = QClipboard::Clipboard;
    qint64 m_lastBellMs = 0; // debounce BEL storms

    int m_cols = 80;
    int m_rows = 24;
    int m_cellWidthPx = 10;
    int m_cellHeightPx = 20;

    // Cached render state + persistent frame pixmap. paintEvent() only blits
    // the pixmap; libghostty work happens when PTY data or a full-invalidate
    // (resize/theme/font/show) requires it.
    GhosttyRenderState m_renderState = nullptr;
    QPixmap m_frameCache;
    bool m_needsRender = true;
    bool m_fullRepaintPending = true;

    // Scrollbar overlay bound to GHOSTTY_TERMINAL_DATA_SCROLLBAR.
    QScrollBar* m_scrollBar = nullptr;
    uint64_t m_scrollTotal = 0;
    uint64_t m_scrollOffset = 0;
    uint64_t m_scrollLen = 0;
    bool m_viewportPinned = true;

    // Selection auto-scroll while dragging near the vertical edges.
    QTimer* m_autoScrollTimer = nullptr;
    int m_autoScrollDir = 0;    // -1 = top edge, +1 = bottom edge, 0 = none
    int m_lastMouseX = 0;       // mouse x, for extending the selection to the edge row

    QMutex m_terminalMutex;
};

#endif // TERMINALBACKEND_GHOSTTY_H
