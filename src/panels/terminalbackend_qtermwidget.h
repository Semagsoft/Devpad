#ifndef TERMINALBACKEND_QTERMWIDGET_H
#define TERMINALBACKEND_QTERMWIDGET_H

#include "terminalbackend.h"

class QTermWidget;

class TerminalBackendQTermWidget : public TerminalBackend
{
    Q_OBJECT

public:
    explicit TerminalBackendQTermWidget(QWidget* parent = nullptr);
    ~TerminalBackendQTermWidget() override;

    void start() override;
    void stop() override;
    void setWorkingDirectory(const QString& path) override;
    void setTerminalFont(const QFont& font) override;
    void applyTheme(const QString& schemeName) override;
    void copyClipboard() override;
    void pasteClipboard() override;
    void sendText(const QString& text) override;
    bool isRunning() const override;

    QTermWidget* widget() const
    {
        return m_term;
    }

private:
    QTermWidget* m_term;
    bool m_running;
    QString m_workingDir;
    QFont m_cachedFont;
};

#endif
