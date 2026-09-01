/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primoterminal.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

static PrimoTerminal* s_instance = nullptr;

PrimoTerminal* PrimoTerminal::instance()
{
    if (!s_instance)
    {
        s_instance = new PrimoTerminal(QCoreApplication::instance());
    }
    return s_instance;
}

PrimoTerminal::PrimoTerminal(QObject* parent) : QObject(parent)
{
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &PrimoTerminal::onReadyRead);
    connect(m_proc, &QProcess::readyReadStandardError, this, &PrimoTerminal::onReadyRead);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PrimoTerminal::onFinished);
    m_currentDir = QDir::homePath();
    m_shell = detectShell();
}

PrimoTerminal::~PrimoTerminal()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning)
    {
        m_proc->terminate();
        m_proc->waitForFinished(500);
        if (m_proc->state() != QProcess::NotRunning)
            m_proc->kill();
    }
}

QString PrimoTerminal::detectShell() const
{
#ifdef Q_OS_WIN
    QString comspec = qEnvironmentVariable("COMSPEC");
    if (!comspec.isEmpty() && QFileInfo::exists(comspec))
        return comspec;
    return QStringLiteral("cmd.exe");
#else
    QString shell = qEnvironmentVariable("SHELL");
    if (!shell.isEmpty() && QFileInfo::exists(shell))
        return shell;
    // Fallback simple shell
    if (QFileInfo::exists(QStringLiteral("/bin/bash")))
        return QStringLiteral("/bin/bash");
    if (QFileInfo::exists(QStringLiteral("/bin/sh")))
        return QStringLiteral("/bin/sh");
    return QStringLiteral("/bin/bash");
#endif
}

QString PrimoTerminal::output() const
{
    return m_output;
}
QString PrimoTerminal::currentDir() const
{
    return m_currentDir;
}
void PrimoTerminal::setCurrentDir(const QString& dir)
{
    if (m_currentDir == dir)
        return;
    QDir d(dir);
    if (!d.exists())
        return;
    m_currentDir = d.absolutePath();
    if (m_proc && m_proc->state() != QProcess::NotRunning)
    {
        // Change dir via cd command
        write(QStringLiteral("cd \"") + m_currentDir + QStringLiteral("\"\n"));
    }
    else
    {
        m_proc->setWorkingDirectory(m_currentDir);
    }
    emit currentDirChanged();
}
bool PrimoTerminal::isRunning() const
{
    return m_running;
}
QString PrimoTerminal::shellProgram() const
{
    return m_shell;
}

void PrimoTerminal::start()
{
    if (m_running)
        return;
    m_proc->setWorkingDirectory(m_currentDir);
#ifdef Q_OS_WIN
    m_proc->setProgram(m_shell);
    m_proc->setArguments({});
#else
    m_proc->setProgram(m_shell);
    m_proc->setArguments(QStringList() << QStringLiteral("-i"));
#endif
    m_proc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    appendOutput(QStringLiteral("$ ") + m_shell + QStringLiteral(" started in ") + m_currentDir + QStringLiteral("\n"));
    m_proc->start();
    if (m_proc->waitForStarted(2000))
    {
        m_running = true;
        emit runningChanged();
        emit shellProgramChanged();
    }
    else
    {
        appendOutput(QStringLiteral("Failed to start shell: ") + m_proc->errorString() + QStringLiteral("\n"));
    }
}

void PrimoTerminal::stop()
{
    if (!m_running)
        return;
    m_proc->terminate();
    if (!m_proc->waitForFinished(1000))
        m_proc->kill();
    m_running = false;
    emit runningChanged();
}

void PrimoTerminal::restart()
{
    stop();
    QTimer::singleShot(200, this, &PrimoTerminal::start);
}

void PrimoTerminal::write(const QString& text)
{
    if (!m_running)
        start();
    if (!m_proc || m_proc->state() == QProcess::NotRunning)
        return;
    // Ensure newline handling: QProcess expects bytes
    QByteArray data = text.toUtf8();
    // If not ending with newline, keep as is for interactive
    m_proc->write(data);
    // Echo input to output for simple shell
    // Do not duplicate if shell will echo; for simple we append
    // appendOutput(text); // shell will echo, so skip
}

void PrimoTerminal::clearOutput()
{
    m_output.clear();
    emit outputChanged();
}

void PrimoTerminal::onReadyRead()
{
    QByteArray data = m_proc->readAll();
    QString txt = QString::fromLocal8Bit(data);
    // Normalize newlines
    txt.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    appendOutput(txt);
    emit outputReceived(txt);
}

void PrimoTerminal::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    m_running = false;
    appendOutput(QStringLiteral("\n[Process finished with exit code ") + QString::number(exitCode) + QStringLiteral("]\n"));
    emit runningChanged();
}

void PrimoTerminal::appendOutput(const QString& text)
{
    m_output += text;
    // Keep output bounded: trim oldest if > 500KB
    if (m_output.size() > 500 * 1024)
    {
        m_output = m_output.right(400 * 1024);
    }
    emit outputChanged();
}
