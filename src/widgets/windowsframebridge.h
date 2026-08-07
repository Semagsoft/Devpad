#ifndef WINDOWSFRAMEBRIDGE_H
#define WINDOWSFRAMEBRIDGE_H

#include <QAbstractNativeEventFilter>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class QWidget;
class TitleBar;

// Bridges native Windows behaviors onto the custom titlebar while keeping the
// native window frame (shadow, resize, snap, system menu) intact. The frame is
// hidden by handling WM_NCCALCSIZE so the client area fills the window, and
// WM_NCHITTEST keeps native dragging, edge resizing, and Aero snap working.
// Compiled as a no-op on non-Windows platforms.
#ifdef Q_OS_WIN
class WindowsFrameBridge : public QAbstractNativeEventFilter
{
public:
    WindowsFrameBridge(QWidget* window, TitleBar* titleBar);
    ~WindowsFrameBridge() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void refreshFrame();
    void showSystemMenu(const POINT& screenPos);
    void setMaxButtonHovered(bool hovered);
    bool isOverMaximizeButton(const POINT& screenPos) const;

    QWidget* m_window = nullptr;
    TitleBar* m_titleBar = nullptr;
    HWND m_hwnd = nullptr;
    bool m_maxButtonHovered = false;
    bool m_maxButtonPressed = false;
};
#endif // Q_OS_WIN

#endif // WINDOWSFRAMEBRIDGE_H
