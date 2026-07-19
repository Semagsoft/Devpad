#ifndef TERMINALBACKEND_H
#define TERMINALBACKEND_H

#include <QWidget>

class TerminalBackend : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalBackend(QWidget* parent = nullptr) : QWidget(parent)
    {
    }
    ~TerminalBackend() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setWorkingDirectory(const QString& path) = 0;
    virtual void setTerminalFont(const QFont& font) = 0;
    virtual void applyTheme(const QString& schemeName) = 0;
    virtual void copyClipboard() = 0;
    virtual void pasteClipboard() = 0;
    virtual void sendText(const QString& text) = 0;
    virtual bool isRunning() const = 0;

signals:
    void finished(int exitCode, int exitStatus);
};

#endif
