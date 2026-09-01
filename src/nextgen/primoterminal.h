/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoTerminal: shared QProcess shell for nextgen QML.
 * Simple TextArea shell via QProcess, shared process across windows.
 * Respects SettingsManager::terminalFont and uses $SHELL or bash/cmd.
 */

#ifndef PRIMOTERMINAL_H
#define PRIMOTERMINAL_H

#include <QObject>
#include <QProcess>
#include <QString>

class PrimoTerminal : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString shellProgram READ shellProgram NOTIFY shellProgramChanged)

public:
    static PrimoTerminal* instance();
    explicit PrimoTerminal(QObject* parent = nullptr);
    ~PrimoTerminal() override;

    QString output() const;
    QString currentDir() const;
    void setCurrentDir(const QString& dir);
    bool isRunning() const;
    QString shellProgram() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void write(const QString& text);
    Q_INVOKABLE void clearOutput();
    Q_INVOKABLE void restart();

signals:
    void outputChanged();
    void currentDirChanged();
    void runningChanged();
    void shellProgramChanged();
    void outputReceived(const QString& text);

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    QString detectShell() const;
    void appendOutput(const QString& text);

    QProcess* m_proc = nullptr;
    QString m_output;
    QString m_currentDir;
    QString m_shell;
    bool m_running = false;
};

#endif // PRIMOTERMINAL_H
