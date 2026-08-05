#ifndef WINDOWSFRAMEBRIDGE_H
#define WINDOWSFRAMEBRIDGE_H

#include <QAbstractNativeEventFilter>

class QWidget;
class TitleBar;

// Bridges native Windows behaviors onto the frameless main window so it keeps
// native dragging, edge resizing, and Aero snap while the menu lives in the
// custom titlebar. Compiled as a no-op on non-Windows platforms.
#ifdef Q_OS_WIN
class WindowsFrameBridge : public QAbstractNativeEventFilter
{
public:
    WindowsFrameBridge(QWidget* window, TitleBar* titleBar);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    QWidget* m_window = nullptr;
    TitleBar* m_titleBar = nullptr;
};
#endif // Q_OS_WIN

#endif // WINDOWSFRAMEBRIDGE_H
