#include "terminalbackend_kodoterm.h"

#include <KodoTerm/KodoTerm.hpp>
#include <KodoTerm/KodoTermConfig.hpp>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QKeyEvent>
#include <QProcessEnvironment>
#include <QVBoxLayout>

TerminalBackendKodoTerm::TerminalBackendKodoTerm(QWidget* parent) : TerminalBackend(parent), m_term(nullptr), m_running(false)
{
}

TerminalBackendKodoTerm::~TerminalBackendKodoTerm()
{
    stop();
}

QString TerminalBackendKodoTerm::shellProgram()
{
#ifdef Q_OS_WIN
    return "cmd.exe";
#else
    return "/bin/sh";
#endif
}

QStringList TerminalBackendKodoTerm::shellArguments()
{
    return {};
}

void TerminalBackendKodoTerm::start()
{
    if (m_running)
        return;
    if (!m_term)
    {
        m_term = new KodoTerm(this);
        if (!m_workingDir.isEmpty())
            m_term->setWorkingDirectory(m_workingDir);
        KodoTermConfig cfg = m_term->getConfig();
        cfg.font = m_cachedFont;
        m_term->setConfig(cfg);
        connect(m_term, &KodoTerm::finished, this,
                [this](int code, int status)
                {
                    m_running = false;
                    emit finished(code, status);
                });
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_term, 1);
        setLayout(layout);
    }

    m_term->setProgram(shellProgram());
    m_term->setArguments(shellArguments());
    m_term->start(true);
    m_running = true;
    m_term->setFocus();
}

void TerminalBackendKodoTerm::stop()
{
    if (!m_term)
        return;
    m_term->kill();
    delete m_term;
    m_term = nullptr;
    m_running = false;
}

void TerminalBackendKodoTerm::setWorkingDirectory(const QString& path)
{
    m_workingDir = path;
    if (m_term)
        m_term->setWorkingDirectory(path);
}

void TerminalBackendKodoTerm::setTerminalFont(const QFont& font)
{
    m_cachedFont = font;
    if (!m_term)
        return;
    KodoTermConfig cfg = m_term->getConfig();
    cfg.font = font;
    m_term->setConfig(cfg);
}

void TerminalBackendKodoTerm::applyTheme(const QString& schemeName)
{
    if (!m_term)
        return;
    Q_UNUSED(schemeName)
    TerminalTheme theme;
    if (schemeName.contains("Dark", Qt::CaseInsensitive) || schemeName.contains("Nord", Qt::CaseInsensitive) ||
        schemeName.contains("Monokai", Qt::CaseInsensitive) || schemeName.contains("Gruvbox", Qt::CaseInsensitive) ||
        schemeName.contains("Mocha", Qt::CaseInsensitive) || schemeName.contains("Macchiato", Qt::CaseInsensitive) ||
        schemeName.contains("Dracula", Qt::CaseInsensitive) || schemeName.contains("OneDark", Qt::CaseInsensitive) ||
        schemeName.contains("TokyoNight", Qt::CaseInsensitive) || schemeName.contains("AyuDark", Qt::CaseInsensitive))
    {
        theme = TerminalTheme::defaultTheme();
        theme.background = QColor("#1e1e2e");
        theme.foreground = QColor("#cdd6f4");
    }
    else
    {
        theme = TerminalTheme::defaultTheme();
        theme.background = QColor("#ffffff");
        theme.foreground = QColor("#1e1e2e");
    }
    m_term->setTheme(theme);
}

void TerminalBackendKodoTerm::copyClipboard()
{
    if (m_term)
        m_term->copyToClipboard();
}

void TerminalBackendKodoTerm::pasteClipboard()
{
    if (m_term)
        m_term->pasteFromClipboard();
}

void TerminalBackendKodoTerm::sendText(const QString& text)
{
    if (!m_term || !m_running)
        return;
    for (int i = 0; i < text.size(); ++i)
    {
        QChar ch = text[i];
        if (ch == QLatin1Char('\n'))
        {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
            QCoreApplication::sendEvent(m_term, &press);
            QCoreApplication::sendEvent(m_term, &release);
        }
        else
        {
            QKeyEvent press(QEvent::KeyPress, -1, Qt::NoModifier, QString(ch));
            QKeyEvent release(QEvent::KeyRelease, -1, Qt::NoModifier, QString(ch));
            QCoreApplication::sendEvent(m_term, &press);
            QCoreApplication::sendEvent(m_term, &release);
        }
    }
}

bool TerminalBackendKodoTerm::isRunning() const
{
    return m_running;
}
