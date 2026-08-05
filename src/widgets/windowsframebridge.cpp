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
#include <QWidget>
#include <initializer_list>

#include <windows.h>
#include <windowsx.h>

namespace
{
constexpr int ResizeBorder = 8;
} // namespace

WindowsFrameBridge::WindowsFrameBridge(QWidget* window, TitleBar* titleBar) : m_window(window), m_titleBar(titleBar)
{
    qApp->installNativeEventFilter(this);
}

bool WindowsFrameBridge::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(eventType)

    if (!m_window->isVisible() || m_window->isFullScreen())
        return false;

    MSG* msg = static_cast<MSG*>(message);
    if (msg->hwnd != reinterpret_cast<HWND>(m_window->winId()))
        return false;

    if (msg->message != WM_NCHITTEST)
        return false;

    const POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};

    // Native edge resizing.
    RECT winRect;
    if (GetWindowRect(msg->hwnd, &winRect))
    {
        const LONG width = winRect.right - winRect.left;
        const LONG height = winRect.bottom - winRect.top;
        const LONG x = pt.x - winRect.left;
        const LONG y = pt.y - winRect.top;

        if (y < ResizeBorder && x < ResizeBorder)
        {
            *result = HTTOPLEFT;
            return true;
        }
        if (y < ResizeBorder && x >= width - ResizeBorder)
        {
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

    // Caption drag over the title bar, keeping interactive children clickable.
    if (m_titleBar && m_titleBar->isVisible())
    {
        const QPoint globalPt(pt.x, pt.y);
        const QRect titleRect(m_titleBar->mapToGlobal(QPoint(0, 0)), m_titleBar->size());
        if (titleRect.contains(globalPt))
        {
            const std::initializer_list<QWidget*> interactive = {
                m_titleBar->menuBarWidget(),
                m_titleBar->minimizeButton(),
                m_titleBar->maximizeButton(),
                m_titleBar->closeButton(),
            };
            for (QWidget* child : interactive)
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

    return false;
}
#endif // Q_OS_WIN
