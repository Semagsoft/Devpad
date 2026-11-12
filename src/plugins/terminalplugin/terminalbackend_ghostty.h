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
#include <QFont>
#include <QKeyEvent>
#include <QMutex>
#include <QPixmap>
#include <QString>
#include <QThread>
#include <QWidget>

#include <ghostty/vt.h>

/**
 * TerminalBackend backed by libghostty-vt (ghostty.org).
 *
 * libghostty owns the VT parser + terminal state; this backend owns the
 * PTY (shell process) and the rendering surface. The backend renders into a
 * persistent frame pixmap; paintEvent() only blits the cached frame.
 */
class QShowEvent;

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
    void sendText(const QString& text) override;
    bool isRunning() const override;
    QWidget* terminalWidget() const override;

    void keyPressEvent(QKeyEvent* event) override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void handlePtyData(const QByteArray& data);
    void handlePtyEof();

private:
    // -- libghostty C API callbacks ----------------------------------------
    static void writePtyCallback(GhosttyTerminal terminal, void* userdata, const uint8_t* data, size_t len);
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

    // Last selection captured during rendering (copy source).
    QString m_selectedText;

    // Shell exit status reported through finished().
    int m_exitCode = -1;
    int m_exitStatus = -1;

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

    QMutex m_terminalMutex;
};

#endif // TERMINALBACKEND_GHOSTTY_H
