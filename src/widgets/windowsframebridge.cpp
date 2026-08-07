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

#include "windowsframebridge.h"

#ifdef Q_OS_WIN
#include "titlebar.h"

#include <QApplication>
#include <QEvent>
#include <QMenuBar>
#include <QPointer>
#include <QToolButton>
#include <QWidget>
#include <initializer_list>

#include <windows.h>
#include <windowsx.h>

namespace
{
constexpr int ResizeBorder = 8;

// Invisible border reserved by Windows around a maximized window for resizing.
int FrameThickness()
{
    return GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
}
} // namespace

WindowsFrameBridge::WindowsFrameBridge(QWidget* window, TitleBar* titleBar) : m_window(window), m_titleBar(titleBar)
{
    // Cache the native handle once. Calling winId() from inside
    // nativeEventFilter would re-enter window creation and recurse.
    m_hwnd = reinterpret_cast<HWND>(m_window->winId());

    qApp->installNativeEventFilter(this);

    // Re-evaluate the frame so WM_NCCALCSIZE removes it immediately.
    refreshFrame();
}

WindowsFrameBridge::~WindowsFrameBridge()
{
    // Restore the native frame when the custom titlebar goes away.
    refreshFrame();
}

void WindowsFrameBridge::refreshFrame()
{
    if (!m_hwnd)
        return;

    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

bool WindowsFrameBridge::isOverMaximizeButton(const POINT& screenPos) const
{
    if (!m_titleBar || !m_titleBar->maximizeButton() || !m_titleBar->maximizeButton()->isVisible())
        return false;

    const QToolButton* button = m_titleBar->maximizeButton();
    const QRect rect(button->mapToGlobal(QPoint(0, 0)), button->size());
    return rect.contains(QPoint(screenPos.x, screenPos.y));
}

void WindowsFrameBridge::setMaxButtonHovered(bool hovered)
{
    if (m_maxButtonHovered == hovered || !m_titleBar || !m_titleBar->maximizeButton())
        return;

    m_maxButtonHovered = hovered;
    QEvent* event = hovered ? static_cast<QEvent*>(new QEvent(QEvent::Enter)) : static_cast<QEvent*>(new QEvent(QEvent::Leave));
    QApplication::sendEvent(m_titleBar->maximizeButton(), event);
    delete event;
}

bool WindowsFrameBridge::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(eventType)

    MSG* msg = static_cast<MSG*>(message);
    if (msg->hwnd != m_hwnd)
        return false;

    // WM_NCCALCSIZE must be handled even before the window is visible: the
    // bridge is created during MainWindow construction (before show()), and
    // only handling it here removes the native frame from the first paint.
    if (msg->message == WM_NCCALCSIZE)
    {
        if (m_window->isFullScreen())
            return false;

        // Hide the native frame: the client area fills the whole window. The
        // non-zero return applies our modified client rect (rgrc[0]).
        RECT* rect = msg->wParam ? &(reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam)->rgrc[0])
                                 : reinterpret_cast<RECT*>(msg->lParam);
        if (IsZoomed(msg->hwnd))
        {
            const int thickness = FrameThickness();
            rect->top += thickness;
            rect->left += thickness;
            rect->right -= thickness;
            rect->bottom -= thickness;
        }
        *result = msg->wParam ? WVR_REDRAW : 0;
        return true;
    }

    if (!m_window->isVisible() || m_window->isFullScreen())
        return false;

    switch (msg->message)
    {
    case WM_NCHITTEST:
    {
        const POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        const bool zoomed = IsZoomed(msg->hwnd);

        // Native edge resizing. Disabled while maximized so the invisible
        // resize borders do not turn the window edges into resize cursors.
        if (!zoomed)
        {
            RECT winRect;
            if (GetWindowRect(msg->hwnd, &winRect))
            {
                const LONG width = winRect.right - winRect.left;
                const LONG height = winRect.bottom - winRect.top;
                const LONG x = pt.x - winRect.left;
                const LONG y = pt.y - winRect.top;

                if (y < ResizeBorder && x < ResizeBorder)
                {
                    setMaxButtonHovered(false);
                    *result = HTTOPLEFT;
                    return true;
                }
                if (y < ResizeBorder && x >= width - ResizeBorder)
                {
                    setMaxButtonHovered(false);
                    *result = HTTOPRIGHT;
                    return true;
                }
                if (y >= height - ResizeBorder && x < ResizeBorder)
                {
                    *result = HTBOTTOMLEFT;
                    return true;
                }
                if (y >= height - ResizeBorder && x >= width - ResizeBorder)
                {
                    *result = HTBOTTOMRIGHT;
                    return true;
                }
                if (y < ResizeBorder)
                {
                    setMaxButtonHovered(false);
                    *result = HTTOP;
                    return true;
                }
                if (y >= height - ResizeBorder)
                {
                    *result = HTBOTTOM;
                    return true;
                }
                if (x < ResizeBorder)
                {
                    *result = HTLEFT;
                    return true;
                }
                if (x >= width - ResizeBorder)
                {
                    *result = HTRIGHT;
                    return true;
                }
            }
        }

        // Caption drag over the title bar, keeping interactive children clickable.
        if (m_titleBar && m_titleBar->isVisible())
        {
            const QPoint globalPt(pt.x, pt.y);
            const QRect titleRect(m_titleBar->mapToGlobal(QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(globalPt))
            {
                // Hitting the maximize/restore button as a native caption button
                // gives the Windows 11 snap-layout flyout and native toggling.
                if (isOverMaximizeButton(pt))
                {
                    setMaxButtonHovered(true);
                    *result = HTMAXBUTTON;
                    return true;
                }
                setMaxButtonHovered(false);

                QPointer<QWidget> interactive[] = {
                    m_titleBar->menuBarWidget(),
                    m_titleBar->minimizeButton(),
                    m_titleBar->closeButton(),
                };
                for (QPointer<QWidget> child : interactive)
                {
                    if (child && child->isVisible())
                    {
                        const QRect childRect(child->mapToGlobal(QPoint(0, 0)), child->size());
                        if (childRect.contains(globalPt))
                        {
                            *result = HTCLIENT;
                            return true;
                        }
                    }
                }

                *result = HTCAPTION;
                return true;
            }
        }

        setMaxButtonHovered(false);
        return false;
    }
    case WM_NCLBUTTONDOWN:
        // Swallow the native press over the maximize button so Windows does not
        // render the classic caption button; reflect the state on our button.
        if (msg->wParam == HTMAXBUTTON)
        {
            m_maxButtonPressed = true;
            m_titleBar->maximizeButton()->setDown(true);
            return true;
        }
        return false;
    case WM_NCLBUTTONUP:
        if (m_maxButtonPressed)
        {
            m_maxButtonPressed = false;
            QToolButton* button = m_titleBar->maximizeButton();
            button->setDown(false);
            if (msg->wParam == HTMAXBUTTON)
                button->click();
            return true;
        }
        if (msg->wParam == HTMAXBUTTON)
            return true;
        return false;
    case WM_NCLBUTTONDBLCLK:
        // Our single-click handling already toggles; swallow the double click.
        if (msg->wParam == HTMAXBUTTON)
            return true;
        return false;
    case WM_NCRBUTTONUP:
        // Right-click on the (non-client) title bar shows the system menu.
        if (msg->wParam == HTCAPTION)
        {
            const POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
            showSystemMenu(pt);
            return true;
        }
        return false;
    case WM_SYSKEYDOWN:
        // Alt+Space opens the system menu at the window's top-left corner.
        if (msg->wParam == VK_SPACE)
        {
            RECT winRect;
            if (GetWindowRect(msg->hwnd, &winRect))
            {
                const POINT pt = {winRect.left, winRect.top};
                showSystemMenu(pt);
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

void WindowsFrameBridge::showSystemMenu(const POINT& screenPos)
{
    HWND hwnd = m_hwnd;
    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (!menu)
        return;

    const bool zoomed = IsZoomed(hwnd);
    EnableMenuItem(menu, SC_RESTORE, MF_BYCOMMAND | (zoomed ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(menu, SC_MOVE, MF_BYCOMMAND | (zoomed ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(menu, SC_SIZE, MF_BYCOMMAND | (zoomed ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(menu, SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(menu, SC_MAXIMIZE, MF_BYCOMMAND | (zoomed ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);

    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, screenPos.x,
                                       screenPos.y, 0, hwnd, nullptr);
    if (command != 0)
        SendMessageW(hwnd, WM_SYSCOMMAND, command, 0);
}
#endif // Q_OS_WIN
