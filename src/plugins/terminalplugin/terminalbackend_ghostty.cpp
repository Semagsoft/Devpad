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

#include "terminalbackend_ghostty.h"

#include "settingsmanager.h"
#include "theme.h"

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMutex>
#include <QPainter>
#include <QRect>
#include <QThread>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace
{
// C ABI callback type used by GHOSTTY_TERMINAL_OPT_WRITE_PTY.
typedef void (*GhosttyWritePtyFn)(GhosttyTerminal terminal, void* userdata, const uint8_t* data, size_t len);

// IO thread: pty master -> ghostty terminal (via queued slot call).
class GhosttyIoThread : public QThread
{
public:
    GhosttyIoThread(TerminalBackendGhostty* backend, int fd, QObject* parent = nullptr) : QThread(parent), m_backend(backend), m_fd(fd)
    {
        if (::pipe(m_wakePipe) != 0)
        {
            m_wakePipe[0] = -1;
            m_wakePipe[1] = -1;
        }
    }

    ~GhosttyIoThread() override
    {
        if (m_wakePipe[0] >= 0)
            ::close(m_wakePipe[0]);
        if (m_wakePipe[1] >= 0)
            ::close(m_wakePipe[1]);
    }

    // Wakes the poll() inside run() so it exits promptly — no 4 Hz timer needed.
    void requestStop()
    {
        m_stop = true;
        if (m_wakePipe[1] >= 0)
        {
            const uint8_t byte = 1;
            ssize_t written = ::write(m_wakePipe[1], &byte, sizeof(byte));
            Q_UNUSED(written);
        }
    }

protected:
    void run() override;

private:
    TerminalBackendGhostty* m_backend;
    int m_fd;
    int m_wakePipe[2] = {-1, -1};
    std::atomic<bool> m_stop{false};
};

// Map a Devpad color scheme name (e.g. "DevpadDark") to a ThemeId.
ThemeId themeIdForScheme(const QString& schemeName)
{
    if (schemeName == QLatin1String("DevpadLight"))
        return ThemeId::Light;
    if (schemeName == QLatin1String("DevpadDark"))
        return ThemeId::Dark;
    if (schemeName == QLatin1String("DevpadNord"))
        return ThemeId::Nord;
    if (schemeName == QLatin1String("DevpadSolarizedLight"))
        return ThemeId::SolarizedLight;
    if (schemeName == QLatin1String("DevpadMonokai"))
        return ThemeId::Monokai;
    if (schemeName == QLatin1String("DevpadGruvboxDark"))
        return ThemeId::GruvboxDark;
    if (schemeName == QLatin1String("DevpadCatppuccinMocha"))
        return ThemeId::CatppuccinMocha;
    if (schemeName == QLatin1String("DevpadCatppuccinMacchiato"))
        return ThemeId::CatppuccinMacchiato;
    if (schemeName == QLatin1String("DevpadCatppuccinFrappe"))
        return ThemeId::CatppuccinFrappe;
    if (schemeName == QLatin1String("DevpadCatppuccinLatte"))
        return ThemeId::CatppuccinLatte;
    if (schemeName == QLatin1String("DevpadTokyoNight"))
        return ThemeId::TokyoNight;
    if (schemeName == QLatin1String("DevpadTokyoNightStorm"))
        return ThemeId::TokyoNightStorm;
    if (schemeName == QLatin1String("DevpadDracula"))
        return ThemeId::Dracula;
    if (schemeName == QLatin1String("DevpadOneDark"))
        return ThemeId::OneDark;
    if (schemeName == QLatin1String("DevpadAyuLight"))
        return ThemeId::AyuLight;
    if (schemeName == QLatin1String("DevpadAyuDark"))
        return ThemeId::AyuDark;
    return ThemeId::Dark;
}

// Resolve a style color (fg/bg/underline) into an RGB value.
GhosttyColorRgb resolveStyleColor(const GhosttyStyleColor& color, const GhosttyRenderStateColors& colors, const GhosttyColorRgb& fallback)
{
    switch (color.tag)
    {
    case GHOSTTY_STYLE_COLOR_RGB:
        return color.value.rgb;
    case GHOSTTY_STYLE_COLOR_PALETTE:
        return colors.palette[color.value.palette];
    default:
        return fallback;
    }
}

} // namespace

void GhosttyIoThread::run()
{
    // One blocking poll on two fds: the PTY master (data to read) and the
    // wake-up pipe (stop requested). The thread consumes zero CPU until the
    // shell writes something or requestStop() wakes it. If the pipe could not
    // be created, fall back to a 250 ms timeout plus the atomic stop flag.
    const bool haveWakePipe = m_wakePipe[0] >= 0 && m_wakePipe[1] >= 0;
    pollfd fds[2];
    fds[0].fd = m_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = m_wakePipe[0];
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    while (true)
    {
        fds[0].revents = 0;
        fds[1].revents = 0;
        int ready = ::poll(fds, haveWakePipe ? 2 : 1, haveWakePipe ? -1 : 250);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (haveWakePipe && (fds[1].revents & (POLLIN | POLLHUP | POLLERR)))
        {
            uint8_t wakeBuf[32];
            ssize_t drained = ::read(m_wakePipe[0], wakeBuf, sizeof(wakeBuf));
            Q_UNUSED(drained);
            break;
        }
        if (!haveWakePipe && m_stop.load(std::memory_order_relaxed))
            break;

        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR))
        {
            uint8_t buf[4096];
            ssize_t n = ::read(m_fd, buf, sizeof(buf));
            if (n < 0)
            {
                if (errno == EINTR)
                    continue;
                break;
            }
            if (n == 0)
            {
                // EOF on the master side: shell has exited.
                QMetaObject::invokeMethod(m_backend, "handlePtyEof", Qt::QueuedConnection);
                break;
            }
            QByteArray data(reinterpret_cast<const char*>(buf), static_cast<int>(n));
            QMetaObject::invokeMethod(m_backend, "handlePtyData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
        }
    }
}

TerminalBackendGhostty::TerminalBackendGhostty(QWidget* parent)
    : TerminalBackend(parent), m_terminal(nullptr), m_keyEncoder(nullptr), m_masterFd(-1), m_pid(-1), m_ioThread(nullptr), m_stopRequested(false),
      m_workingDir(QString()), m_cachedFont(QFont()), m_running(false), m_cols(80), m_rows(24), m_cellWidthPx(10), m_cellHeightPx(20)
{
    setFocusPolicy(Qt::StrongFocus);
    // We always paint our own background; Qt must not erase/under-paint.
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

TerminalBackendGhostty::~TerminalBackendGhostty()
{
    stop();
}

void TerminalBackendGhostty::start()
{
    if (m_running)
        return;

    int cols = m_cols > 0 ? m_cols : 80;
    int rows = m_rows > 0 ? m_rows : 24;

    GhosttyResult res = ghostty_terminal_new(nullptr, &m_terminal, static_cast<uint16_t>(cols), static_cast<uint16_t>(rows));
    if (res != GHOSTTY_SUCCESS || m_terminal == nullptr)
    {
        m_running = false;
        return;
    }
    if (ghostty_render_state_new(nullptr, &m_renderState) != GHOSTTY_SUCCESS)
    {
        ghostty_terminal_free(m_terminal);
        m_terminal = nullptr;
        m_running = false;
        return;
    }
    m_needsRender = true;
    m_fullRepaintPending = true;

    // Theme colors.
    ThemeColors colors = getThemeColors(SettingsManager::instance().theme());
    GhosttyColorRgb background;
    background.r = static_cast<uint8_t>(colors.background.red());
    background.g = static_cast<uint8_t>(colors.background.green());
    background.b = static_cast<uint8_t>(colors.background.blue());
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_BACKGROUND, &background);

    GhosttyColorRgb foreground;
    foreground.r = static_cast<uint8_t>(colors.foreground.red());
    foreground.g = static_cast<uint8_t>(colors.foreground.green());
    foreground.b = static_cast<uint8_t>(colors.foreground.blue());
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_FOREGROUND, &foreground);

    GhosttyColorRgb cursorColor;
    cursorColor.r = static_cast<uint8_t>(colors.caret.red());
    cursorColor.g = static_cast<uint8_t>(colors.caret.green());
    cursorColor.b = static_cast<uint8_t>(colors.caret.blue());
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_CURSOR, &cursorColor);

    GhosttyColorRgb palette[256];
    ghostty_color_palette_default(palette);
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_PALETTE, palette);

    GhosttyCursorStyle cursorStyle = GHOSTTY_CURSOR_STYLE_BLOCK;
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_DEFAULT_CURSOR_STYLE, &cursorStyle);
    bool cursorBlink = true;
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_DEFAULT_CURSOR_BLINK, &cursorBlink);

    // Host -> pty output (responses to queries etc.).
    GhosttyWritePtyFn writePtyFn = &TerminalBackendGhostty::writePtyCallback;
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_WRITE_PTY, reinterpret_cast<const void*>(writePtyFn));

    // Key encoder, synced to the terminal state (legacy modes by default).
    if (ghostty_key_encoder_new(nullptr, &m_keyEncoder) == GHOSTTY_SUCCESS)
    {
        ghostty_key_encoder_set_from_terminal(m_keyEncoder, m_terminal);
    }

    resizeTerminal();

    if (!startPty())
    {
        if (m_terminal)
        {
            ghostty_terminal_free(m_terminal);
            m_terminal = nullptr;
        }
        if (m_keyEncoder)
        {
            ghostty_key_encoder_free(m_keyEncoder);
            m_keyEncoder = nullptr;
        }
        return;
    }

    startIoThread();
    m_running = true;
}

void TerminalBackendGhostty::stop()
{
    stopIoThread();
    stopPty();

    QMutexLocker locker(&m_terminalMutex);
    if (m_terminal)
    {
        ghostty_terminal_reset(m_terminal);
        ghostty_terminal_free(m_terminal);
        m_terminal = nullptr;
    }
    if (m_keyEncoder)
    {
        ghostty_key_encoder_free(m_keyEncoder);
        m_keyEncoder = nullptr;
    }
    if (m_renderState)
    {
        ghostty_render_state_free(m_renderState);
        m_renderState = nullptr;
    }
    m_frameCache = QPixmap();
    m_needsRender = true;
    m_fullRepaintPending = true;
    m_running = false;
}

void TerminalBackendGhostty::setWorkingDirectory(const QString& path)
{
    m_workingDir = path;
}

void TerminalBackendGhostty::setTerminalFont(const QFont& font)
{
    m_cachedFont = font;
    resizeTerminal();
    m_needsRender = true;
    m_fullRepaintPending = true;
    update();
}

void TerminalBackendGhostty::applyTheme(const QString& schemeName)
{
    if (!m_running || !m_terminal)
        return;
    ThemeColors colors = getThemeColors(themeIdForScheme(schemeName));
    GhosttyColorRgb background;
    background.r = static_cast<uint8_t>(colors.background.red());
    background.g = static_cast<uint8_t>(colors.background.green());
    background.b = static_cast<uint8_t>(colors.background.blue());
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_BACKGROUND, &background);

    GhosttyColorRgb foreground;
    foreground.r = static_cast<uint8_t>(colors.foreground.red());
    foreground.g = static_cast<uint8_t>(colors.foreground.green());
    foreground.b = static_cast<uint8_t>(colors.foreground.blue());
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_FOREGROUND, &foreground);

    GhosttyColorRgb cursorColor;
    cursorColor.r = static_cast<uint8_t>(colors.caret.red());
    cursorColor.g = static_cast<uint8_t>(colors.caret.green());
    cursorColor.b = static_cast<uint8_t>(colors.caret.blue());
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_COLOR_CURSOR, &cursorColor);

    m_needsRender = true;
    m_fullRepaintPending = true;
    update();
}

void TerminalBackendGhostty::copyClipboard()
{
    QApplication::clipboard()->setText(m_selectedText);
}

void TerminalBackendGhostty::pasteClipboard()
{
    if (!m_running || !m_terminal)
        return;

    static const char kMimeType[] = "text/plain";

    GhosttyPaste paste = GHOSTTY_INIT_SIZED(GhosttyPaste);
    paste.size = sizeof(GhosttyPaste);
    paste.location = GHOSTTY_CLIPBOARD_LOCATION_PRIMARY;
    paste.source = GHOSTTY_PASTE_SOURCE_CLIPBOARD;
    GhosttyString mimeEntries[1];
    mimeEntries[0].ptr = reinterpret_cast<const uint8_t*>(kMimeType);
    mimeEntries[0].len = static_cast<size_t>(std::strlen(kMimeType));
    paste.mimes = mimeEntries;
    paste.mimes_len = 1;
    paste.reader.read = &TerminalBackendGhostty::ghosttyMimeReader;
    paste.reader.userdata = this;
    paste.allow_unsafe = true;

    bool pasted = false;
    GhosttyResult result = ghostty_terminal_paste(m_terminal, &paste, &pasted);
    Q_UNUSED(result);
}

void TerminalBackendGhostty::sendText(const QString& text)
{
    if (!m_running || !m_terminal)
        return;

    QMutexLocker locker(&m_terminalMutex);
    QByteArray utf8 = text.toUtf8();
    if (utf8.isEmpty())
        return;
    ghostty_terminal_vt_write(m_terminal, reinterpret_cast<const uint8_t*>(utf8.constData()), utf8.size());
}

bool TerminalBackendGhostty::isRunning() const
{
    return m_running;
}

QWidget* TerminalBackendGhostty::terminalWidget() const
{
    return const_cast<TerminalBackendGhostty*>(this);
}

void TerminalBackendGhostty::keyPressEvent(QKeyEvent* event)
{
    if (!m_running || !m_terminal)
        return;

    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    // Ctrl+PageUp / Ctrl+Insert copy the selection; Shift+PageUp / Shift+Insert paste.
    if (key == Qt::Key_PageUp)
    {
        if (modifiers.testFlag(Qt::ControlModifier))
            copyClipboard();
        else if (modifiers.testFlag(Qt::ShiftModifier))
            pasteClipboard();
        event->accept();
        return;
    }
    if (key == Qt::Key_Insert)
    {
        if (modifiers.testFlag(Qt::ShiftModifier))
            pasteClipboard();
        else if (modifiers.testFlag(Qt::ControlModifier))
            copyClipboard();
        event->accept();
        return;
    }

    GhosttyMods ghosttyMods = 0;
    if (modifiers & Qt::ShiftModifier)
        ghosttyMods |= GHOSTTY_MODS_SHIFT;
    if (modifiers & Qt::ControlModifier)
        ghosttyMods |= GHOSTTY_MODS_CTRL;
    if (modifiers & Qt::AltModifier)
        ghosttyMods |= GHOSTTY_MODS_ALT;
    if (modifiers & Qt::MetaModifier)
        ghosttyMods |= GHOSTTY_MODS_SUPER;

    GhosttyKey ghosttyKey = GHOSTTY_KEY_UNIDENTIFIED;
    QString text = event->text();
    uint32_t unshifted = 0;

    switch (key)
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        ghosttyKey = GHOSTTY_KEY_ENTER;
        break;
    case Qt::Key_Backspace:
        ghosttyKey = GHOSTTY_KEY_BACKSPACE;
        break;
    case Qt::Key_Tab:
        ghosttyKey = GHOSTTY_KEY_TAB;
        break;
    case Qt::Key_Escape:
        ghosttyKey = GHOSTTY_KEY_ESCAPE;
        break;
    case Qt::Key_Space:
        ghosttyKey = GHOSTTY_KEY_SPACE;
        break;
    case Qt::Key_Home:
        ghosttyKey = GHOSTTY_KEY_HOME;
        break;
    case Qt::Key_End:
        ghosttyKey = GHOSTTY_KEY_END;
        break;
    case Qt::Key_PageUp:
        ghosttyKey = GHOSTTY_KEY_PAGE_UP;
        break;
    case Qt::Key_PageDown:
        ghosttyKey = GHOSTTY_KEY_PAGE_DOWN;
        break;
    case Qt::Key_Insert:
        ghosttyKey = GHOSTTY_KEY_INSERT;
        break;
    case Qt::Key_Delete:
        ghosttyKey = GHOSTTY_KEY_DELETE;
        break;
    case Qt::Key_Left:
        ghosttyKey = GHOSTTY_KEY_ARROW_LEFT;
        break;
    case Qt::Key_Right:
        ghosttyKey = GHOSTTY_KEY_ARROW_RIGHT;
        break;
    case Qt::Key_Up:
        ghosttyKey = GHOSTTY_KEY_ARROW_UP;
        break;
    case Qt::Key_Down:
        ghosttyKey = GHOSTTY_KEY_ARROW_DOWN;
        break;
    case Qt::Key_F1:
        ghosttyKey = GHOSTTY_KEY_F1;
        break;
    case Qt::Key_F2:
        ghosttyKey = GHOSTTY_KEY_F2;
        break;
    case Qt::Key_F3:
        ghosttyKey = GHOSTTY_KEY_F3;
        break;
    case Qt::Key_F4:
        ghosttyKey = GHOSTTY_KEY_F4;
        break;
    case Qt::Key_F5:
        ghosttyKey = GHOSTTY_KEY_F5;
        break;
    case Qt::Key_F6:
        ghosttyKey = GHOSTTY_KEY_F6;
        break;
    case Qt::Key_F7:
        ghosttyKey = GHOSTTY_KEY_F7;
        break;
    case Qt::Key_F8:
        ghosttyKey = GHOSTTY_KEY_F8;
        break;
    case Qt::Key_F9:
        ghosttyKey = GHOSTTY_KEY_F9;
        break;
    case Qt::Key_F10:
        ghosttyKey = GHOSTTY_KEY_F10;
        break;
    case Qt::Key_F11:
        ghosttyKey = GHOSTTY_KEY_F11;
        break;
    case Qt::Key_F12:
        ghosttyKey = GHOSTTY_KEY_F12;
        break;
    default:
        break;
    }

    if (text.size() > 0)
    {
        QChar c = text.at(0);
        uint32_t uc = c.unicode();
        if (uc >= 'a' && uc <= 'z')
        {
            ghosttyKey = static_cast<GhosttyKey>(GHOSTTY_KEY_A + (uc - 'a'));
            unshifted = uc;
        }
        else if (uc >= 'A' && uc <= 'Z')
        {
            ghosttyKey = static_cast<GhosttyKey>(GHOSTTY_KEY_A + (uc - 'A'));
            unshifted = uc;
        }
        else if (uc >= '0' && uc <= '9')
        {
            ghosttyKey = static_cast<GhosttyKey>(GHOSTTY_KEY_DIGIT_0 + (uc - '0'));
            unshifted = uc;
        }
        else if (uc == '`')
        {
            ghosttyKey = GHOSTTY_KEY_BACKQUOTE;
            unshifted = uc;
        }
        else if (uc == 92) // backslash
        {
            ghosttyKey = GHOSTTY_KEY_BACKSLASH;
            unshifted = uc;
        }
        else if (uc == '[')
        {
            ghosttyKey = GHOSTTY_KEY_BRACKET_LEFT;
            unshifted = uc;
        }
        else if (uc == ']')
        {
            ghosttyKey = GHOSTTY_KEY_BRACKET_RIGHT;
            unshifted = uc;
        }
        else if (uc == ';')
        {
            ghosttyKey = GHOSTTY_KEY_SEMICOLON;
            unshifted = uc;
        }
        else if (uc == '\'')
        {
            ghosttyKey = GHOSTTY_KEY_QUOTE;
            unshifted = uc;
        }
        else if (uc == '/')
        {
            ghosttyKey = GHOSTTY_KEY_SLASH;
            unshifted = uc;
        }
        else if (uc == '.')
        {
            ghosttyKey = GHOSTTY_KEY_PERIOD;
            unshifted = uc;
        }
        else if (uc == '-')
        {
            ghosttyKey = GHOSTTY_KEY_MINUS;
            unshifted = uc;
        }
        else if (uc == '=')
        {
            ghosttyKey = GHOSTTY_KEY_EQUAL;
            unshifted = uc;
        }
        else if (uc == ',')
        {
            ghosttyKey = GHOSTTY_KEY_COMMA;
            unshifted = uc;
        }
        if ((modifiers & Qt::ShiftModifier) && (uc >= 'a' && uc <= 'z'))
            unshifted = uc - 'a' + 'A';
    }

    QByteArray utf8 = text.toUtf8();
    GhosttyKeyEvent eventHandle;
    GhosttyResult eventResult = ghostty_key_event_new(nullptr, &eventHandle);
    if (eventResult != GHOSTTY_SUCCESS)
        return;
    ghostty_key_event_set_action(eventHandle, GHOSTTY_KEY_ACTION_PRESS);
    ghostty_key_event_set_key(eventHandle, ghosttyKey);
    ghostty_key_event_set_mods(eventHandle, ghosttyMods);
    ghostty_key_event_set_consumed_mods(eventHandle, 0);
    ghostty_key_event_set_composing(eventHandle, false);
    GhosttyString utf8Value;
    utf8Value.ptr = reinterpret_cast<const uint8_t*>(utf8.constData());
    utf8Value.len = utf8.size();
    ghostty_key_event_set_utf8(eventHandle, utf8Value);
    ghostty_key_event_set_unshifted_codepoint(eventHandle, unshifted);

    uint8_t keyBuffer[256];
    size_t written = 0;
    GhosttyResult encodeResult = ghostty_key_encoder_encode(m_keyEncoder, eventHandle, keyBuffer, sizeof(keyBuffer), &written);
    ghostty_key_event_free(eventHandle);
    if (encodeResult == GHOSTTY_SUCCESS && written > 0)
        writeToMaster(keyBuffer, written);
}

void TerminalBackendGhostty::handlePtyData(const QByteArray& data)
{
    QMutexLocker locker(&m_terminalMutex);
    if (!m_terminal || data.isEmpty())
        return;
    ghostty_terminal_vt_write(m_terminal, reinterpret_cast<const uint8_t*>(data.constData()), data.size());
    m_needsRender = true; // Dirty rows only; renderFrame decides full vs dirty from m_fullRepaintPending.
    update(); // Repaint so newly arrived PTY data is visible on the widget.
}

void TerminalBackendGhostty::handlePtyEof()
{
    if (m_pid > 0)
    {
        int status = 0;
        if (::waitpid(m_pid, &status, 0) >= 0 && WIFEXITED(status))
        {
            m_exitCode = WEXITSTATUS(status);
            m_exitStatus = 0;
        }
        else
        {
            m_exitStatus = status;
        }
        m_pid = -1;
    }
    m_running = false;
    emit finished(m_exitCode, m_exitStatus);
}

void TerminalBackendGhostty::resizeTerminal()
{
    if (!m_terminal)
        return;
    QFontMetrics metrics(m_cachedFont);
    int cellW = metrics.horizontalAdvance(QChar('x'));
    int cellH = int(metrics.ascent()) + int(metrics.descent());
    if (cellW < 1)
        cellW = 1;
    if (cellH < 1)
        cellH = 1;
    m_cellWidthPx = cellW;
    m_cellHeightPx = cellH;
    int cols = m_cols > 0 ? m_cols : 80;
    int rows = m_rows > 0 ? m_rows : 24;
    ghostty_terminal_resize(m_terminal, static_cast<uint16_t>(cols), static_cast<uint16_t>(rows), static_cast<uint32_t>(cellW),
                            static_cast<uint32_t>(cellH));
    // Tell the PTY (and therefore the shell) its new size; otherwise bash
    // keeps the 80x24 default and wraps/misaligns output.
    if (m_masterFd >= 0)
    {
        struct winsize ws;
        ws.ws_row = static_cast<unsigned short>(rows);
        ws.ws_col = static_cast<unsigned short>(cols);
        ws.ws_xpixel = static_cast<unsigned short>(cols * cellW);
        ws.ws_ypixel = static_cast<unsigned short>(rows * cellH);
        ::ioctl(m_masterFd, TIOCSWINSZ, &ws);
    }
}

void TerminalBackendGhostty::startIoThread()
{
    m_stopRequested = false;
    GhosttyIoThread* thread = new GhosttyIoThread(this, m_masterFd);
    thread->start();
    m_ioThread = thread;
}

void TerminalBackendGhostty::stopIoThread()
{
    if (m_ioThread)
    {
        auto ioThread = static_cast<GhosttyIoThread*>(m_ioThread);
        ioThread->requestStop(); // Wakes the blocking poll() in the IO thread immediately.
        m_ioThread->wait(3000);
        delete m_ioThread;
        m_ioThread = nullptr;
    }
}

void TerminalBackendGhostty::stopPty()
{
    if (m_masterFd >= 0)
    {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
    if (m_pid > 0)
    {
        ::kill(m_pid, SIGKILL);
        int status = 0;
        while (::waitpid(m_pid, &status, 0) < 0 && errno == EINTR)
        {
        }
        m_pid = -1;
    }
}

void TerminalBackendGhostty::writeToMaster(const uint8_t* data, size_t len)
{
    if (m_masterFd < 0 || !data || len == 0)
        return;
    ssize_t written = ::write(m_masterFd, data, len);
    Q_UNUSED(written);
}

void TerminalBackendGhostty::renderFrame()
{
    // Paint into the persistent frame pixmap only; every other paint path just
    // blits it (see paintEvent).
    QMutexLocker locker(&m_terminalMutex);
    if (!m_terminal || !m_renderState)
    {
        m_needsRender = false;
        m_fullRepaintPending = false;
        return;
    }

    bool fullRepaint = m_fullRepaintPending;
    QRect frameRect = rect();
    if (frameRect.isEmpty())
    {
        m_needsRender = false;
        return;
    }

    if (m_frameCache.size() != frameRect.size())
    {
        m_frameCache = QPixmap(frameRect.size());
        fullRepaint = true; // Fresh pixmap must be fully redrawn.
    }

    GhosttyResult result = ghostty_render_state_update(m_renderState, m_terminal);
    if (result != GHOSTTY_SUCCESS)
        return;

    GhosttyRenderStateColors colors = GHOSTTY_INIT_SIZED(GhosttyRenderStateColors);
    ghostty_render_state_get(m_renderState, GHOSTTY_RENDER_STATE_DATA_COLORS, &colors);

    QColor backgroundColor(static_cast<int>(colors.background.r), static_cast<int>(colors.background.g), static_cast<int>(colors.background.b));

    QPainter painter(&m_frameCache);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, false);
    if (fullRepaint)
        painter.fillRect(frameRect, backgroundColor);
    painter.setFont(m_cachedFont);

    GhosttyRenderStateCursor cursor = GHOSTTY_INIT_SIZED(GhosttyRenderStateCursor);
    ghostty_render_state_get(m_renderState, GHOSTTY_RENDER_STATE_DATA_CURSOR, &cursor);

    GhosttyRenderStateRowIterator rowIterator = nullptr;
    if (ghostty_render_state_row_iterator_new(nullptr, &rowIterator) == GHOSTTY_SUCCESS)
    {
        ghostty_render_state_get(m_renderState, GHOSTTY_RENDER_STATE_DATA_ROW_ITERATOR, &rowIterator);

        // The cells handle must be created up front; row_get() populates it,
        // it does not allocate it. Passing nullptr would yield zero cells.
        GhosttyRenderStateRowCells cells = nullptr;
        if (ghostty_render_state_row_cells_new(nullptr, &cells) != GHOSTTY_SUCCESS)
        {
            ghostty_render_state_row_iterator_free(rowIterator);
        }
        else
        {
        uint16_t rowY = 0;
        bool rowHasValue = false;
        while (true)
        {
            if (fullRepaint)
                rowHasValue = ghostty_render_state_row_iterator_next(rowIterator);
            else
                rowHasValue = ghostty_render_state_row_iterator_next_dirty(rowIterator, &rowY);
            if (!rowHasValue)
                break;

            // Clear the row strip so stale glyphs/backgrounds/cursors are erased.
            if (!fullRepaint)
                painter.fillRect(QRect(0, rowY * m_cellHeightPx, frameRect.width(), m_cellHeightPx), backgroundColor);

            GhosttyResult cellsResult = ghostty_render_state_row_get(rowIterator, GHOSTTY_RENDER_STATE_ROW_DATA_CELLS, &cells);
            if (cellsResult == GHOSTTY_SUCCESS)
            {
                int column = 0;
                while (ghostty_render_state_row_cells_next(cells))
                {
                    QRectF cellRect(QPointF(column * m_cellWidthPx, rowY * m_cellHeightPx), QSizeF(m_cellWidthPx, m_cellHeightPx));

                    GhosttyStyle style;
                    ghostty_render_state_row_cells_get(cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_STYLE, &style);

                    // Fill the cell background first so it does not cover the text.
                    GhosttyColorRgb cellBackground;
                    if (ghostty_render_state_row_cells_get(cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_BG_COLOR, &cellBackground) == GHOSTTY_SUCCESS)
                    {
                        QColor backgroundCell(static_cast<int>(cellBackground.r), static_cast<int>(cellBackground.g),
                                              static_cast<int>(cellBackground.b));
                        painter.fillRect(cellRect, backgroundCell);
                    }

                    uint32_t graphemesLen = 0;
                    ghostty_render_state_row_cells_get(cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_LEN, &graphemesLen);

                    if (graphemesLen > 0)
                    {
                        uint32_t codepoints[16];
                        GhosttyCodepointBuffer graphemes;
                        graphemes.ptr = codepoints;
                        graphemes.cap = 16;
                        graphemes.len = 0;
                        ghostty_render_state_row_cells_get(cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_BUF, &graphemes);
                        size_t codepointCount = graphemes.len < graphemes.cap ? graphemes.len : graphemes.cap;
                        QString text;
                        for (size_t i = 0; i < codepointCount; ++i)
                        {
                            QChar c(static_cast<ushort>(codepoints[i]));
                            text.append(c);
                        }

                        GhosttyColorRgb cellForeground = resolveStyleColor(style.fg_color, colors, colors.foreground);
                        QColor foregroundColor(static_cast<int>(cellForeground.r), static_cast<int>(cellForeground.g),
                                               static_cast<int>(cellForeground.b));
                        QFont cellFont = m_cachedFont;
                        if (style.flags & GHOSTTY_STYLE_FLAG_BOLD)
                            cellFont.setBold(true);
                        painter.setFont(cellFont);

                        // Text is drawn with the pen, not the brush.
                        painter.setPen(QPen(foregroundColor));
                        QTextOption textOption;
                        textOption.setAlignment(Qt::AlignTop | Qt::AlignLeft);
                        painter.drawText(cellRect, text, textOption);
                    }

                    int underline = GHOSTTY_STYLE_UNDERLINE(style);
                    if (underline != GHOSTTY_SGR_UNDERLINE_NONE)
                    {
                        GhosttyColorRgb cellForeground = resolveStyleColor(style.fg_color, colors, colors.foreground);
                        GhosttyColorRgb underlineColor = resolveStyleColor(style.underline_color, colors, cellForeground);
                        QColor underlineColorC(static_cast<int>(underlineColor.r), static_cast<int>(underlineColor.g),
                                               static_cast<int>(underlineColor.b));
                        painter.setPen(QPen(underlineColorC));
                        painter.drawLine(QPointF(cellRect.topLeft().x(), cellRect.top() + cellRect.height()),
                                         QPointF(cellRect.topLeft().x() + cellRect.width(), cellRect.top() + cellRect.height()));
                    }
                    column++;
                }
            }
            if (fullRepaint)
                rowY++;
        }
        ghostty_render_state_row_cells_free(cells);
        }
        ghostty_render_state_row_iterator_free(rowIterator);
    }

    ghostty_render_state_clean(m_renderState);

    if (cursor.visible && cursor.viewport_has_value)
    {
        int cursorX = static_cast<int>(cursor.viewport_x);
        int cursorY = static_cast<int>(cursor.viewport_y);
        QRectF cursorRect(QPointF(cursorX * m_cellWidthPx, cursorY * m_cellHeightPx), QSizeF(m_cellWidthPx, m_cellHeightPx));
        QColor cursorColorC(static_cast<int>(colors.cursor.r), static_cast<int>(colors.cursor.g), static_cast<int>(colors.cursor.b));
        switch (cursor.visual_style)
        {
        case GHOSTTY_CURSOR_STYLE_BLOCK:
        case GHOSTTY_CURSOR_STYLE_BLOCK_HOLLOW:
            painter.fillRect(cursorRect, cursorColorC);
            break;
        case GHOSTTY_CURSOR_STYLE_UNDERLINE:
            painter.setPen(QPen(cursorColorC));
            painter.drawLine(QPointF(cursorRect.topLeft().x(), cursorRect.top() + cursorRect.height()),
                             QPointF(cursorRect.topLeft().x() + cursorRect.width(), cursorRect.top() + cursorRect.height()));
            break;
        case GHOSTTY_CURSOR_STYLE_BAR:
            painter.setPen(QPen(cursorColorC));
            painter.drawLine(QPointF(cursorRect.left() + cursorRect.width() / 2, cursorRect.top()),
                             QPointF(cursorRect.left() + cursorRect.width() / 2, cursorRect.top() + cursorRect.height()));
            break;
        default:
            painter.fillRect(cursorRect, cursorColorC);
            break;
        }
    }

    m_needsRender = false;
    m_fullRepaintPending = false;
}

void TerminalBackendGhostty::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (m_needsRender)
        renderFrame();

    QPainter painter(this);
    if (!m_frameCache.isNull())
        painter.drawPixmap(0, 0, m_frameCache);
    else
        painter.fillRect(rect(), QColor(Qt::black));
}

void TerminalBackendGhostty::updateGridSize(const QSize& size)
{
    if (!m_running || !m_terminal)
        return;
    if (size.width() < 20 || size.height() < 10)
        return; // Collapsed/hidden layout: keep the last good grid.

    QFontMetrics metrics(m_cachedFont);
    int cellW = metrics.horizontalAdvance(QChar('x'));
    int cellH = int(metrics.ascent()) + int(metrics.descent());
    if (cellW < 1)
        cellW = 1;
    if (cellH < 1)
        cellH = 1;
    int cols = size.width() / cellW;
    int rows = size.height() / cellH;
    if (cols < 2)
        cols = 2;
    if (rows < 2)
        rows = 2;
    if (cols == m_cols && rows == m_rows)
        return; // No change.
    m_cols = cols;
    m_rows = rows;
    resizeTerminal();
    // Cell metrics/grid changed: drop the stale frame and force a full redraw.
    m_frameCache = QPixmap();
    m_needsRender = true;
    m_fullRepaintPending = true;
    update();
}

void TerminalBackendGhostty::resizeEvent(QResizeEvent* event)
{
    TerminalBackend::resizeEvent(event);
    // While hidden/collapsed we get degenerate sizes; keep the last good grid
    // and let showEvent() re-sync.
    if (!isVisible())
        return;
    updateGridSize(event->size());
}

void TerminalBackendGhostty::showEvent(QShowEvent* event)
{
    TerminalBackend::showEvent(event);
    // Re-sync the grid/pixmap with our current size — but only re-render when
    // the size actually changed (updateGridSize bumps the repaint flags only
    // then). Otherwise this is just a cheap blit of the cached frame.
    updateGridSize(size());
    update();
}

bool TerminalBackendGhostty::ghosttyMimeReader(void* userdata, GhosttyString mime, GhosttyWriter writer)
{
    TerminalBackendGhostty* self = static_cast<TerminalBackendGhostty*>(userdata);
    if (!self)
        return false;
    QString text = QApplication::clipboard()->text();
    QByteArray utf8 = text.toUtf8();
    if (utf8.isEmpty())
        return false;
    std::string mimeType(reinterpret_cast<const char*>(mime.ptr), static_cast<size_t>(mime.len));
    if (mimeType.compare(0, 5, "text/") != 0)
        return false;
    bool ok = writer.write(writer.userdata, reinterpret_cast<const uint8_t*>(utf8.constData()), utf8.size());
    return ok;
}

bool TerminalBackendGhostty::startPty()
{
    int masterFd = -1;
    int slaveFd = -1;
    int cols = m_cols > 0 ? m_cols : 80;
    int rows = m_rows > 0 ? m_rows : 24;
    struct winsize ws;
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);
    ws.ws_xpixel = static_cast<unsigned short>(cols * m_cellWidthPx);
    ws.ws_ypixel = static_cast<unsigned short>(rows * m_cellHeightPx);
    if (openpty(&masterFd, &slaveFd, nullptr, nullptr, &ws) < 0)
        return false;
    m_masterFd = masterFd;

    pid_t pid = ::fork();
    if (pid < 0)
    {
        ::close(masterFd);
        ::close(slaveFd);
        m_masterFd = -1;
        return false;
    }
    if (pid == 0)
    {
        // Child: exec the user's shell in the new pty.
        ::close(masterFd);
        ::setsid();
        if (slaveFd >= 0)
        {
            ::ioctl(slaveFd, TIOCSCTTY, 0);
            // Attach stdio to the PTY so the shell gets a controlling terminal
            // and does not complain about job control.
            ::dup2(slaveFd, STDIN_FILENO);
            ::dup2(slaveFd, STDOUT_FILENO);
            ::dup2(slaveFd, STDERR_FILENO);
            if (slaveFd > STDERR_FILENO)
                ::close(slaveFd);
        }
        const char* shell = ::getenv("SHELL");
        if (!shell || shell[0] == '\0')
            shell = "/bin/bash";
        if (!m_workingDir.isEmpty())
        {
            ::chdir(m_workingDir.toLocal8Bit().constData());
        }
        else
        {
            const char* home = ::getenv("HOME");
            if (home && home[0] != '\0')
                ::chdir(home);
        }
        ::setenv("TERM", "xterm-256color", 1);
        ::execl(shell, shell, static_cast<char*>(nullptr));
        ::_exit(127);
    }

    if (slaveFd >= 0)
        ::close(slaveFd);
    m_pid = pid;
    return true;
}

void TerminalBackendGhostty::writePtyCallback(GhosttyTerminal terminal, void* userdata, const uint8_t* data, size_t len)
{
    Q_UNUSED(terminal);
    TerminalBackendGhostty* self = static_cast<TerminalBackendGhostty*>(userdata);
    if (self)
        self->writeToMaster(data, len);
}
