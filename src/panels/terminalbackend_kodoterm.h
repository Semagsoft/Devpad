#ifndef TERMINALBACKEND_KODOTERM_H
#define TERMINALBACKEND_KODOTERM_H

#include "terminalbackend.h"

#ifdef Q_OS_WIN
#include <KodoTerm/KodoTerm.hpp>
#endif

class TerminalBackendKodoTerm : public TerminalBackend
{
    Q_OBJECT

public:
    explicit TerminalBackendKodoTerm(QWidget* parent = nullptr);
    ~TerminalBackendKodoTerm() override;

    void start() override;
    void stop() override;
    void setWorkingDirectory(const QString& path) override;
    void setTerminalFont(const QFont& font) override;
    void applyTheme(const QString& schemeName) override;
    void copyClipboard() override;
    void pasteClipboard() override;
    void sendText(const QString& text) override;
    bool isRunning() const override;

private:
    static QString shellProgram();
    static QStringList shellArguments();

    KodoTerm* m_term;
    bool m_running;
    QString m_workingDir;
    QFont m_cachedFont;
};

#endif
