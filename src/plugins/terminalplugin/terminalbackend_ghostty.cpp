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
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QThread>
#include <QWheelEvent>
#include <atomic>
#include <cerrno>
#include <chrono>
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

    // Thin overlay scrollbar for scrolling the viewport through scrollback.
    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_scrollBar->setObjectName(QStringLiteral("terminalScrollBar"));
    m_scrollBar->setFixedWidth(12);
    m_scrollBar->setVisible(false);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value)
            {
                if (!m_running || !m_terminal)
                    return;
                GhosttyTerminalScrollViewport behavior = {};
                behavior.tag = GHOSTTY_TERMINAL_SCROLL_VIEWPORT_ROW;
                behavior.value.row = static_cast<size_t>(qMax(0, value));
                QMutexLocker locker(&m_terminalMutex);
                ghostty_terminal_scroll_viewport(m_terminal, behavior);
                m_needsRender = true;
                m_fullRepaintPending = true;
                update();
            });

    // Edge-proximity auto-scroll while dragging a selection.
    m_autoScrollTimer = new QTimer(this);
    m_autoScrollTimer->setInterval(50);
    connect(m_autoScrollTimer, &QTimer::timeout, this, &TerminalBackendGhostty::autoScrollTick);
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
    // Retain a couple of pages of scrollback so long output isn't lost.
    size_t maxScrollbackLines = 10000;
    ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_SCROLLBACK_MAX_LINES, &maxScrollbackLines);
    m_needsRender = true;
    m_fullRepaintPending = true;

    // Mouse selection gesture and its reusable press/drag/release events.
    if (ghostty_selection_gesture_new(nullptr, &m_selectionGesture) != GHOSTTY_SUCCESS)
        m_selectionGesture = nullptr;
    if (m_selectionGesture)
    {
        if (ghostty_selection_gesture_event_new(nullptr, &m_selectPressEvent, GHOSTTY_SELECTION_GESTURE_EVENT_TYPE_PRESS) != GHOSTTY_SUCCESS)
            m_selectPressEvent = nullptr;
        if (ghostty_selection_gesture_event_new(nullptr, &m_selectDragEvent, GHOSTTY_SELECTION_GESTURE_EVENT_TYPE_DRAG) != GHOSTTY_SUCCESS)
            m_selectDragEvent = nullptr;
        if (ghostty_selection_gesture_event_new(nullptr, &m_selectReleaseEvent, GHOSTTY_SELECTION_GESTURE_EVENT_TYPE_RELEASE) != GHOSTTY_SUCCESS)
            m_selectReleaseEvent = nullptr;
    }

    // Theme colors.
    ThemeColors colors = getThemeColors(SettingsManager::instance().theme());
    m_selectionBg = colors.selectionBg;
    m_selectionFg = colors.selectionFg;
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
        if (m_selectPressEvent)
        {
            ghostty_selection_gesture_event_free(m_selectPressEvent);
            m_selectPressEvent = nullptr;
        }
        if (m_selectDragEvent)
        {
            ghostty_selection_gesture_event_free(m_selectDragEvent);
            m_selectDragEvent = nullptr;
        }
        if (m_selectReleaseEvent)
        {
            ghostty_selection_gesture_event_free(m_selectReleaseEvent);
            m_selectReleaseEvent = nullptr;
        }
        if (m_selectionGesture)
        {
            ghostty_selection_gesture_free(m_selectionGesture, m_terminal);
            m_selectionGesture = nullptr;
        }
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

    if (m_selectPressEvent)
    {
        ghostty_selection_gesture_event_free(m_selectPressEvent);
        m_selectPressEvent = nullptr;
    }
    if (m_selectDragEvent)
    {
        ghostty_selection_gesture_event_free(m_selectDragEvent);
        m_selectDragEvent = nullptr;
    }
    if (m_selectReleaseEvent)
    {
        ghostty_selection_gesture_event_free(m_selectReleaseEvent);
        m_selectReleaseEvent = nullptr;
    }
    if (m_selectionGesture)
    {
        ghostty_selection_gesture_free(m_selectionGesture, m_terminal);
        m_selectionGesture = nullptr;
    }
    m_selecting = false;
    m_autoScrollDir = 0;
    if (m_autoScrollTimer)
        m_autoScrollTimer->stop();
    if (m_scrollBar)
        m_scrollBar->setVisible(false);

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
    m_selectionBg = colors.selectionBg;
    m_selectionFg = colors.selectionFg;

    m_needsRender = true;
    m_fullRepaintPending = true;
    update();
}

void TerminalBackendGhostty::copyClipboard()
{
    if (!m_running || !m_terminal)
        return;

    // The formatter can export the terminal's active selection to plain text.
    GhosttySelection selection = {};
    if (ghostty_terminal_get(m_terminal, GHOSTTY_TERMINAL_DATA_SELECTION, &selection) != GHOSTTY_SUCCESS)
        return; // GHOSTTY_NO_VALUE: no selection

    GhosttyFormatter formatter = nullptr;
    if (ghostty_formatter_terminal_new(nullptr, &formatter, m_terminal) != GHOSTTY_SUCCESS)
        return;

    bool unwrap = true;
    bool trim = true;
    GhosttyFormatterFormat formatMode = GHOSTTY_FORMATTER_FORMAT_PLAIN;
    ghostty_formatter_set(formatter, GHOSTTY_FORMATTER_OPT_UNWRAP, &unwrap);
    ghostty_formatter_set(formatter, GHOSTTY_FORMATTER_OPT_TRIM, &trim);
    ghostty_formatter_set(formatter, GHOSTTY_FORMATTER_OPT_EMIT, &formatMode);
    ghostty_formatter_set(formatter, GHOSTTY_FORMATTER_OPT_SELECTION, &selection);

    uint8_t* buf = nullptr;
    size_t len = 0;
    if (ghostty_formatter_format_alloc(formatter, nullptr, &buf, &len) == GHOSTTY_SUCCESS && buf != nullptr && len > 0)
        QApplication::clipboard()->setText(QString::fromUtf8(reinterpret_cast<const char*>(buf), static_cast<int>(len)));
    ghostty_free(nullptr, buf, len);
    ghostty_formatter_free(formatter);
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
    if (key == Qt::Key_C && modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::ShiftModifier))
    {
        copyClipboard();
        event->accept();
        return;
    }
    if (key == Qt::Key_V && modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::ShiftModifier))
    {
        pasteClipboard();
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

    // Render at the widget's device pixel ratio; otherwise the frame is
    // upscaled by the compositor and text looks blurry/jagged.
    const qreal dpr = devicePixelRatioF();
    if (m_frameCache.size() != (frameRect.size() * dpr) || m_frameCache.devicePixelRatio() != dpr)
    {
        m_frameCache = QPixmap(frameRect.size() * dpr);
        m_frameCache.setDevicePixelRatio(dpr);
        fullRepaint = true; // Fresh pixmap must be fully redrawn.
    }

    GhosttyResult result = ghostty_render_state_update(m_renderState, m_terminal);
    if (result != GHOSTTY_SUCCESS)
        return;

    GhosttyRenderStateColors colors = GHOSTTY_INIT_SIZED(GhosttyRenderStateColors);
    ghostty_render_state_get(m_renderState, GHOSTTY_RENDER_STATE_DATA_COLORS, &colors);

    QColor backgroundColor(static_cast<int>(colors.background.r), static_cast<int>(colors.background.g), static_cast<int>(colors.background.b));

    QPainter painter(&m_frameCache);
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

            GhosttyRenderStateRowSelection rowSelection = GHOSTTY_INIT_SIZED(GhosttyRenderStateRowSelection);
            bool rowSelected =
                ghostty_render_state_row_get(rowIterator, GHOSTTY_RENDER_STATE_ROW_DATA_SELECTION, &rowSelection) == GHOSTTY_SUCCESS;

            GhosttyResult cellsResult = ghostty_render_state_row_get(rowIterator, GHOSTTY_RENDER_STATE_ROW_DATA_CELLS, &cells);
            if (cellsResult == GHOSTTY_SUCCESS)
            {
                int column = 0;
                while (ghostty_render_state_row_cells_next(cells))
                {
                    QRectF cellRect(QPointF(column * m_cellWidthPx, rowY * m_cellHeightPx), QSizeF(m_cellWidthPx, m_cellHeightPx));
                    bool cellSelected = rowSelected && column >= rowSelection.start_x && column <= rowSelection.end_x;

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

                    // Selection overlay, on top of the cell background, under the text.
                    if (cellSelected && m_selectionBg.isValid())
                    {
                        QColor overlay = m_selectionBg;
                        overlay.setAlpha(160);
                        painter.fillRect(cellRect, overlay);
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
                        if (cellSelected && m_selectionFg.isValid())
                            foregroundColor = m_selectionFg;
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

    // Poll viewport/scrollbar state so the overlay scrollbar and the
    // scrolled-up indicator have fresh values.
    GhosttyTerminalScrollbar bar = {};
    if (ghostty_terminal_get(m_terminal, GHOSTTY_TERMINAL_DATA_SCROLLBAR, &bar) == GHOSTTY_SUCCESS)
    {
        m_scrollTotal = bar.total;
        m_scrollOffset = bar.offset;
        m_scrollLen = bar.len;
    }
    bool pinned = m_viewportPinned;
    if (ghostty_terminal_get(m_terminal, GHOSTTY_TERMINAL_DATA_VIEWPORT_ACTIVE, &pinned) == GHOSTTY_SUCCESS)
        m_viewportPinned = pinned;
    updateScrollbar();

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
    // A monitor move can change the device pixel ratio; drop the stale frame.
    if (!m_frameCache.isNull() && m_frameCache.devicePixelRatio() != devicePixelRatioF())
    {
        m_needsRender = true;
        m_fullRepaintPending = true;
    }
    if (m_needsRender)
        renderFrame();

    QPainter painter(this);
    if (!m_frameCache.isNull())
        painter.drawPixmap(0, 0, m_frameCache);
    else
        painter.fillRect(rect(), QColor(Qt::black));

    // "N rows above bottom" indicator when scrolled up into history.
    const uint64_t rowsBelow = (m_scrollOffset + m_scrollLen <= m_scrollTotal) ? (m_scrollTotal - m_scrollOffset - m_scrollLen) : 0;
    if (!m_viewportPinned && rowsBelow > 0)
    {
        const QString text = tr("↑ %1 rows above bottom").arg(rowsBelow);
        QFont indicatorFont = font();
        indicatorFont.setPointSize(9);
        indicatorFont.setBold(true);
        painter.setFont(indicatorFont);
        const QSize textSize = QFontMetrics(indicatorFont).size(Qt::TextSingleLine, text);
        const QRect boxRect(rect().right() - textSize.width() - 28, rect().bottom() - textSize.height() - 16,
                            textSize.width() + 14, textSize.height() + 8);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 170));
        painter.drawRoundedRect(boxRect, 5, 5);
        painter.setPen(QColor(240, 240, 240));
        painter.drawText(boxRect, Qt::AlignCenter, text);
    }
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
    // Position the overlay scrollbar along the right edge.
    if (m_scrollBar)
        m_scrollBar->setGeometry(width() - 12, 0, 12, height());
    // Any size change invalidates the cached frame (even if the grid size
    // didn't change), so the pixmap is recreated at the new size/DPR.
    m_needsRender = true;
    m_fullRepaintPending = true;
    update();
    // While hidden/collapsed we get degenerate sizes; keep the last good grid
    // and let showEvent() re-sync.
    if (!isVisible())
        return;
    updateGridSize(event->size());
}

void TerminalBackendGhostty::showEvent(QShowEvent* event)
{
    TerminalBackend::showEvent(event);
    if (m_scrollBar)
        m_scrollBar->setGeometry(width() - 12, 0, 12, height());
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

uint64_t TerminalBackendGhostty::nowNs() const
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

void TerminalBackendGhostty::positionGestureEvents(const QPoint& pos)
{
    if (!m_terminal)
        return;
    int col = qBound(0, pos.x() / m_cellWidthPx, m_cols - 1);
    int row = qBound(0, pos.y() / m_cellHeightPx, m_rows - 1);

    GhosttyPoint point = {};
    point.tag = GHOSTTY_POINT_TAG_VIEWPORT;
    point.value.coordinate.x = static_cast<uint16_t>(col);
    point.value.coordinate.y = static_cast<uint32_t>(row);
    GhosttyGridRef ref = {};
    if (ghostty_terminal_grid_ref(m_terminal, point, &ref) != GHOSTTY_SUCCESS)
        return;

    GhosttySelectionGestureGeometry geom = {};
    geom.columns = static_cast<uint32_t>(m_cols);
    geom.cell_width = static_cast<uint32_t>(m_cellWidthPx);
    geom.padding_left = 0;
    geom.screen_height = static_cast<uint32_t>(m_rows * m_cellHeightPx);

    uint64_t now = nowNs();
    double repeatDistance = 5.0;
    double repeatIntervalNs = 500000000.0; // 500 ms between repeat clicks

    if (m_selectPressEvent)
    {
        ghostty_selection_gesture_event_set(m_selectPressEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, &ref);
        ghostty_selection_gesture_event_set(m_selectPressEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_GEOMETRY, &geom);
        ghostty_selection_gesture_event_set(m_selectPressEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_TIME_NS, &now);
        ghostty_selection_gesture_event_set(m_selectPressEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REPEAT_DISTANCE, &repeatDistance);
        ghostty_selection_gesture_event_set(m_selectPressEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REPEAT_INTERVAL_NS, &repeatIntervalNs);
    }
    if (m_selectDragEvent)
    {
        ghostty_selection_gesture_event_set(m_selectDragEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, &ref);
        ghostty_selection_gesture_event_set(m_selectDragEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_GEOMETRY, &geom);
        ghostty_selection_gesture_event_set(m_selectDragEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_TIME_NS, &now);
    }
    if (m_selectReleaseEvent)
    {
        ghostty_selection_gesture_event_set(m_selectReleaseEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, &ref);
        ghostty_selection_gesture_event_set(m_selectReleaseEvent, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_GEOMETRY, &geom);
    }
}

void TerminalBackendGhostty::wheelEvent(QWheelEvent* event)
{
    if (!m_running || !m_terminal)
    {
        event->ignore();
        return;
    }

    // If the app inside the terminal enabled mouse tracking, let it see the wheel.
    bool mouseTracking = false;
    ghostty_terminal_get(m_terminal, GHOSTTY_TERMINAL_DATA_MOUSE_TRACKING, &mouseTracking);
    if (mouseTracking)
    {
        event->ignore();
        return;
    }

    // Loose line mapping like Qt does: 120 units per notch, 3 lines per notch.
    int delta = event->angleDelta().y();
    int lines = delta / 40;
    if (lines == 0)
        lines = delta > 0 ? 1 : -1;

    GhosttyTerminalScrollViewport behavior = {};
    behavior.tag = GHOSTTY_TERMINAL_SCROLL_VIEWPORT_DELTA;
    // Negative delta scrolls up into the scrollback history.
    behavior.value.delta = static_cast<intptr_t>(-lines);
    ghostty_terminal_scroll_viewport(m_terminal, behavior);
    m_needsRender = true;
    m_fullRepaintPending = true; // The entire viewport potentially changed.
    update();
    event->accept();
}

void TerminalBackendGhostty::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_running && m_terminal)
    {
        bool mouseTracking = false;
        ghostty_terminal_get(m_terminal, GHOSTTY_TERMINAL_DATA_MOUSE_TRACKING, &mouseTracking);
        if (!mouseTracking)
        {
            positionGestureEvents(event->pos());
            GhosttySelection sel = {};
            if (m_selectionGesture && m_selectPressEvent
                && ghostty_selection_gesture_event(m_selectionGesture, m_terminal, m_selectPressEvent, &sel) == GHOSTTY_SUCCESS)
            {
                m_selecting = true;
                ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_SELECTION, &sel);
            }
            m_needsRender = true;
            m_fullRepaintPending = true;
            update();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TerminalBackendGhostty::mouseMoveEvent(QMouseEvent* event)
{
    if (m_selecting && (event->buttons() & Qt::LeftButton) && m_running && m_terminal)
    {
        positionGestureEvents(event->pos());
        GhosttySelection sel = {};
        if (m_selectionGesture && m_selectDragEvent
            && ghostty_selection_gesture_event(m_selectionGesture, m_terminal, m_selectDragEvent, &sel) == GHOSTTY_SUCCESS)
        {
            ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_SELECTION, &sel);
        }
        m_lastMouseX = event->pos().x();
        const int edgePx = m_cellHeightPx * 2;
        const int y = event->pos().y();
        m_autoScrollDir = (y < edgePx) ? -1 : (y > height() - edgePx ? 1 : 0);
        updateAutoScrollTimer();
        m_needsRender = true;
        m_fullRepaintPending = true;
        update();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TerminalBackendGhostty::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_selecting && m_running && m_terminal)
        {
            positionGestureEvents(event->pos());
            if (m_selectionGesture && m_selectReleaseEvent)
                ghostty_selection_gesture_event(m_selectionGesture, m_terminal, m_selectReleaseEvent, nullptr);
            m_needsRender = true;
            update();
        }
        m_selecting = false;
        m_autoScrollDir = 0;
        updateAutoScrollTimer();
    }
    QWidget::mouseReleaseEvent(event);
}

void TerminalBackendGhostty::updateScrollbar()
{
    if (!m_scrollBar)
        return;
    m_scrollBar->blockSignals(true);
    if (m_scrollTotal <= m_scrollLen || m_scrollLen == 0)
    {
        m_scrollBar->setVisible(false);
    }
    else
    {
        m_scrollBar->setVisible(true);
        m_scrollBar->setRange(0, static_cast<int>(m_scrollTotal - m_scrollLen));
        m_scrollBar->setPageStep(static_cast<int>(m_scrollLen));
        m_scrollBar->setSingleStep(1);
        m_scrollBar->setValue(static_cast<int>(m_scrollOffset));
    }
    m_scrollBar->blockSignals(false);
}

void TerminalBackendGhostty::updateAutoScrollTimer()
{
    if (m_autoScrollDir != 0 && m_selecting)
        m_autoScrollTimer->start();
    else
        m_autoScrollTimer->stop();
}

void TerminalBackendGhostty::autoScrollTick()
{
    if (m_autoScrollDir == 0 || !m_running || !m_terminal || !m_selecting)
    {
        m_autoScrollTimer->stop();
        return;
    }
    const int dir = m_autoScrollDir;
    int edgeRow = dir < 0 ? 0 : m_rows - 1;
    int edgeCol = qBound(0, m_lastMouseX / m_cellWidthPx, m_cols - 1);

    QMutexLocker locker(&m_terminalMutex);
    GhosttyTerminalScrollViewport behavior = {};
    behavior.tag = GHOSTTY_TERMINAL_SCROLL_VIEWPORT_DELTA;
    behavior.value.delta = dir < 0 ? -1 : 1;
    ghostty_terminal_scroll_viewport(m_terminal, behavior);

    if (m_selectionGesture && m_selectDragEvent)
    {
        QPoint pos(edgeCol * m_cellWidthPx, edgeRow * m_cellHeightPx + m_cellHeightPx / 2);
        positionGestureEvents(pos);
        GhosttySelection sel = {};
        if (ghostty_selection_gesture_event(m_selectionGesture, m_terminal, m_selectDragEvent, &sel) == GHOSTTY_SUCCESS)
            ghostty_terminal_set(m_terminal, GHOSTTY_TERMINAL_OPT_SELECTION, &sel);
    }
    m_needsRender = true;
    m_fullRepaintPending = true;
    update();
}
