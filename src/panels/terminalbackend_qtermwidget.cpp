#include "terminalbackend_qtermwidget.h"
#include <QVBoxLayout>
#include <qtermwidget.h>
#include <QStandardPaths>

TerminalBackendQTermWidget::TerminalBackendQTermWidget(QWidget *parent)
    : TerminalBackend(parent), m_term(nullptr), m_running(false)
{
}

TerminalBackendQTermWidget::~TerminalBackendQTermWidget()
{
    stop();
}

void TerminalBackendQTermWidget::start()
{
    if (m_running) return;
    if (!m_term) {
        m_term = new QTermWidget(0, this);
        if (!m_workingDir.isEmpty())
            m_term->setWorkingDirectory(m_workingDir);
        m_term->setHistorySize(-1);
        m_term->setScrollBarPosition(QTermWidgetInterface::ScrollBarRight);
        connect(m_term, &QTermWidget::finished, this, [this]() {
            m_running = false;
            emit finished(0, 0);
        });
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_term, 1);
        setLayout(layout);

        QTermWidget::addCustomColorSchemeDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/devpad/color-schemes");
    }
    m_term->startShellProgram();
    m_running = true;
    m_term->setFocus();
}

void TerminalBackendQTermWidget::stop()
{
    if (!m_term) return;
    delete m_term;
    m_term = nullptr;
    m_running = false;
}

void TerminalBackendQTermWidget::setWorkingDirectory(const QString &path)
{
    m_workingDir = path;
    if (m_term)
        m_term->setWorkingDirectory(path);
}

void TerminalBackendQTermWidget::setTerminalFont(const QFont &font)
{
    if (m_term)
        m_term->setTerminalFont(font);
}

void TerminalBackendQTermWidget::applyTheme(const QString &schemeName)
{
    if (m_term)
        m_term->setColorScheme(schemeName);
}

void TerminalBackendQTermWidget::copyClipboard()
{
    if (m_term)
        m_term->copyClipboard();
}

void TerminalBackendQTermWidget::pasteClipboard()
{
    if (m_term)
        m_term->pasteClipboard();
}

void TerminalBackendQTermWidget::sendText(const QString &text)
{
    if (m_term)
        m_term->sendText(text);
}

bool TerminalBackendQTermWidget::isRunning() const
{
    return m_running;
}
