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
#include "devpad_version.h"
#include "logger.h"
#include "mainwindow.h"
#include "managers/backupmanager.h"
#include "managers/settingsmanager.h"
#include "theme.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QList>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QProcess>
#include <QTextStream>
#include <array>
#include <cstdlib>
#include <iostream>

#ifdef Q_OS_UNIX
#include <csignal>
#include <cstring>

#include <execinfo.h>
#include <unistd.h>

static void crashHandler(int sig, siginfo_t* info, void* userData)
{
    const char* msg = "\n=== CRASH ===\nSignal: ";
    (void)sig;
    (void)info;
    (void)userData;
    if (write(STDERR_FILENO, msg, strlen(msg)) == -1)
    {
    }

    const char* sigName = nullptr;
    switch (sig)
    {
    case SIGSEGV:
        sigName = "SIGSEGV";
        break;
    case SIGABRT:
        sigName = "SIGABRT";
        break;
    case SIGFPE:
        sigName = "SIGFPE";
        break;
    default:
        sigName = "Unknown";
        break;
    }
    if (write(STDERR_FILENO, sigName, strlen(sigName)) == -1)
    {
    }

    if (info && sig == SIGSEGV)
    {
        const char* addrMsg = "\nFault address: ";
        if (write(STDERR_FILENO, addrMsg, strlen(addrMsg)) == -1)
        {
        }
        std::array<char, 32> addr;
        int len = snprintf(addr.data(), addr.size(), "%p", info->si_addr);
        if (write(STDERR_FILENO, addr.data(), static_cast<size_t>(len)) == -1)
        {
        }
    }

    if (write(STDERR_FILENO, "\n\nBacktrace:\n", 12) == -1)
    {
    }

    std::array<void*, 64> buffer;
    int frames = backtrace(buffer.data(), static_cast<int>(buffer.size()));
    backtrace_symbols_fd(buffer.data(), frames, STDERR_FILENO);

    if (write(STDERR_FILENO, "\nRun with: ulimit -c unlimited && gdb ./build/Devpad core\n", 59) == -1)
    {
    }

    _Exit(EXIT_FAILURE);
}
#endif

#ifdef Q_OS_LINUX
// Try to hand off shell-passed files/folders to an already running Devpad
// instance so they open in that instance's current window instead of spawning
// a brand-new window. Returns true if the handoff succeeded and this instance
// should exit without opening anything locally.
static bool handOffToRunningInstance(const QStringList& paths)
{
    if (paths.isEmpty())
        return false;

    QProcess pgrep;
    // -U: only our own uid; do not push file paths into other users' editors.
    pgrep.start(QStringLiteral("pgrep"), QStringList() << QStringLiteral("-ix") << QStringLiteral("-U") << QString::number(static_cast<qint64>(geteuid())) << QStringLiteral("Devpad"));
    if (!pgrep.waitForFinished(2000))
        return false;

    QList<qint64> pids;
    for (const QString& line : QString::fromUtf8(pgrep.readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
    {
        qint64 pid = line.trimmed().toLongLong();
        if (pid > 0 && pid != QCoreApplication::applicationPid())
            pids.append(pid);
    }

    for (qint64 pid : pids)
    {
        QLocalSocket socket;
        socket.connectToServer(QStringLiteral("devpad-%1").arg(pid));
        if (!socket.waitForConnected(1000))
            continue;

        QTextStream stream(&socket);
        stream << QStringLiteral("open_files\n");
        for (const QString& path : paths)
            stream << path << '\n';
        stream << QStringLiteral("end\n");
        stream.flush();

        if (socket.waitForReadyRead(2000))
        {
            while (socket.canReadLine())
            {
                const QString line = QString::fromUtf8(socket.readLine()).trimmed();
                if (line == QStringLiteral("ok"))
                {
                    socket.disconnectFromServer();
                    return true;
                }
            }
        }
        socket.disconnectFromServer();
    }
    return false;
}
#else
static bool handOffToRunningInstance(const QStringList&)
{
    return false;
}
#endif

int main(int argc, char* argv[])
{
#ifdef Q_OS_UNIX
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crashHandler;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
#endif

    // Early parser for --tui / --nextgen to decide application type.
    // We must know before constructing QApplication (which needs a display server).
    bool tuiRequested = false;
    bool nextgenRequested = false;
    for (int i = 1; i < argc; ++i)
    {
        QString a = QString::fromLocal8Bit(argv[i]);
        if (a == QStringLiteral("--tui") || a == QStringLiteral("--no-gui") || a == QStringLiteral("-t"))
        {
            tuiRequested = true;
        }
        if (a == QStringLiteral("--nextgen") || a == QStringLiteral("-n"))
        {
            nextgenRequested = true;
        }
    }
    if (qEnvironmentVariableIsSet("DEVPAD_TUI"))
        tuiRequested = true;
    if (qEnvironmentVariableIsSet("DEVPAD_NEXTGEN"))
        nextgenRequested = true;

    if (tuiRequested && nextgenRequested)
    {
        QTextStream err(stderr);
        err << "Cannot combine --tui and --nextgen. Choose one mode.\n";
        return 1;
    }

    if (nextgenRequested)
    {
#ifdef BUILD_NEXTGEN
        extern int runNextgenApp(QCoreApplication * app, const QCommandLineParser& parser, const QStringList& positional);
        // Use QGuiApplication for QML; QCoreApplication is enough for non-GUI fallback.
        // We use QGuiApplication here; if no display, QML will fail gracefully.
        QGuiApplication app(argc, argv);
        app.setOrganizationName("Semagsoft");
        app.setOrganizationDomain("semagsoft.com");
        app.setApplicationName("Devpad");
        app.setApplicationVersion(DEVPAD_VERSION);

        QCommandLineParser parser;
        parser.setApplicationDescription(QStringLiteral("Devpad - Next-gen QML mode (primoEditor)"));
        parser.addHelpOption();
        parser.addVersionOption();
        QCommandLineOption nextgenOpt(QStringList() << "nextgen" << "n", QStringLiteral("Run in next-gen QML/high-perf mode (primoEditor)"));
        parser.addOption(nextgenOpt);
        QCommandLineOption tuiOpt2(QStringList() << "tui" << "no-gui" << "t", QStringLiteral("Run in terminal UI mode"));
        parser.addOption(tuiOpt2);
        parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Files or folders to open"), QStringLiteral("[files...]"));
        QCommandLineOption transferOpt2(QStringList() << "transfer", QStringLiteral("Open a file transferred from another instance"),
                                        QStringLiteral("file"));
        parser.addOption(transferOpt2);
        QCommandLineOption smokeOpt(QStringList() << "nextgen-smoke-test", QStringLiteral("Load the Nextgen QML UI and exit (for CI)"));
        parser.addOption(smokeOpt);
        QCommandLineOption noSessionOpt2(QStringList() << "no-session", QStringLiteral("Do not restore previous session"));
        parser.addOption(noSessionOpt2);
        parser.process(app);

        QStringList positional = parser.positionalArguments();
        if (parser.isSet(transferOpt2))
        {
            QString v = parser.value(transferOpt2);
            if (!v.isEmpty())
                positional.prepend(v);
        }
        // Cast QGuiApplication to QCoreApplication for unified runNextgenApp signature
        return runNextgenApp(static_cast<QCoreApplication*>(&app), parser, positional);
#else
        QTextStream err(stderr);
        err << "Next-gen mode requested but this build was configured without -DBUILD_NEXTGEN=ON\n";
        err << "Reconfigure with: cmake -S . -B build -DBUILD_NEXTGEN=ON\n";
        return 1;
#endif
    }

    if (tuiRequested)
    {
        QCoreApplication app(argc, argv);
        QCoreApplication::setOrganizationName("Semagsoft");
        QCoreApplication::setOrganizationDomain("semagsoft.com");
        QCoreApplication::setApplicationName("Devpad");
        QCoreApplication::setApplicationVersion(DEVPAD_VERSION);

        QCommandLineParser parser;
        parser.setApplicationDescription(QStringLiteral("Devpad - Terminal UI mode"));
        parser.addHelpOption();
        parser.addVersionOption();
        QCommandLineOption tuiOpt(QStringList() << "tui" << "no-gui" << "t", QStringLiteral("Run in terminal UI mode"));
        parser.addOption(tuiOpt);
        parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Files or folders to open"), QStringLiteral("[files...]"));
        // Also accept --transfer for compatibility but ignore in TUI
        QCommandLineOption transferOpt(QStringList() << "transfer", QStringLiteral("Open a file transferred from another instance"),
                                       QStringLiteral("file"));
        parser.addOption(transferOpt);
        QCommandLineOption noSessionOpt(QStringList() << "no-session", QStringLiteral("Do not restore previous session"));
        parser.addOption(noSessionOpt);
        parser.process(app);

#ifdef BUILD_TUI
        // Defer to TUI backend
        extern int runTuiApp(QCoreApplication * app, const QCommandLineParser& parser, const QStringList& positional);
        QStringList positional = parser.positionalArguments();
        // Filter out --transfer value if present via positional handling already done by parser
        if (parser.isSet(transferOpt))
        {
            QString v = parser.value(transferOpt);
            if (!v.isEmpty())
                positional.prepend(v);
        }
        return runTuiApp(&app, parser, positional);
#else
        QTextStream err(stderr);
        err << "TUI mode requested but this build was configured without -DBUILD_TUI=ON\n";
        err << "Reconfigure with: cmake -S . -B build -DBUILD_TUI=ON\n";
        return 1;
#endif
    }

    // Suppress noisy, non-actionable Qt platform warnings at startup.
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qpa.services=false\nqt.accessibility.atspi=false"));

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Semagsoft");
    QApplication::setOrganizationDomain("semagsoft.com");
    QApplication::setApplicationName("Devpad");
    QApplication::setApplicationVersion(DEVPAD_VERSION);
    QGuiApplication::setDesktopFileName("com.semagsoft.Devpad");

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Devpad - A C++/Qt6 code editor"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption transferOpt(QStringList() << "transfer", QStringLiteral("Open a file transferred from another instance"),
                                   QStringLiteral("file"));
    parser.addOption(transferOpt);
    QCommandLineOption tuiOpt(QStringList() << "tui" << "no-gui" << "t", QStringLiteral("Run in terminal UI mode"));
    parser.addOption(tuiOpt);
    QCommandLineOption nextgenOpt(QStringList() << "nextgen" << "n", QStringLiteral("Run in next-gen QML/high-perf mode (primoEditor)"));
    parser.addOption(nextgenOpt);
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Files or folders to open"), QStringLiteral("[files...]"));
    parser.process(app);

    initThemeSystem();

    {
        const auto& ui = SettingsManager::instance().uiSettings();
        if (!ui.uiFontFamily.isEmpty())
            QApplication::setFont(QFont(ui.uiFontFamily, ui.uiFontSize));
    }

    MainWindow mainWindow;

    // Collect shell-passed files/folders. When "Current window" mode is set
    // and another Devpad instance is already running, hand them off to that
    // instance so they open in its current window instead of spawning a new
    // one. Otherwise they open in this window below.
    QStringList shellFiles;
    QStringList shellFolders;

    const auto collectShellPath = [&](const QString& arg)
    {
        QString filePath = arg;
        if (!QFileInfo(filePath).isAbsolute())
            filePath = QDir::current().absoluteFilePath(filePath);
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile())
            shellFiles.append(filePath);
        else if (fileInfo.isDir())
            shellFolders.append(filePath);
    };

    if (parser.isSet(transferOpt))
    {
        const QString transferPath = parser.value(transferOpt);
        if (!transferPath.isEmpty())
            shellFiles.append(transferPath);
    }

    // Legacy --transfer positional handling for drag-detached tabs
    QStringList positional = parser.positionalArguments();
    for (int i = 0; i < positional.size(); ++i)
    {
        const QString arg = positional.at(i);
        if (arg == QStringLiteral("--transfer") && i + 1 < positional.size())
        {
            shellFiles.append(positional.at(++i));
            continue;
        }
        collectShellPath(arg);
    }

    const bool handoffFiles = SettingsManager::instance().shellOpenModeFiles() == ShellOpenMode::CurrentWindow;
    const bool handoffFolders = SettingsManager::instance().shellOpenModeFolders() == ShellOpenMode::CurrentWindow;
    QStringList handOffPaths;
    if (handoffFiles)
        handOffPaths += shellFiles;
    if (handoffFolders)
        handOffPaths += shellFolders;
    if (!handOffPaths.isEmpty() && handOffToRunningInstance(handOffPaths))
        return 0;

    // Helpers to honor the shell open mode settings when files/folders are
    // passed on the command line. NewWindow launches a detached Devpad window;
    // CurrentWindow opens in the existing window.
    const auto openFileFromShell = [&](const QString& path)
    {
        if (SettingsManager::instance().shellOpenModeFiles() == ShellOpenMode::NewWindow)
        {
            const QString appPath = QApplication::applicationFilePath();
            if (!QProcess::startDetached(appPath, QStringList() << path))
            {
                Logger::instance().error(QString("Failed to start detached process for: %1").arg(path));
            }
        }
        else
        {
            mainWindow.openFileFromPath(path);
        }
    };
    const auto openFolderFromShell = [&](const QString& path)
    {
        if (SettingsManager::instance().shellOpenModeFolders() == ShellOpenMode::NewWindow)
        {
            const QString appPath = QApplication::applicationFilePath();
            if (!QProcess::startDetached(appPath, QStringList() << path))
            {
                Logger::instance().error(QString("Failed to start detached process for folder: %1").arg(path));
            }
        }
        else
        {
            mainWindow.openFolderFromPath(path);
        }
    };

    for (const QString& path : shellFiles)
        openFileFromShell(path);
    for (const QString& path : shellFolders)
        openFolderFromShell(path);

    // Clean up auto-save backups that will never be prompted for again:
    // originals deleted on disk, or backups older than the retention window.
    BackupManager::purgeStaleBackups();

    mainWindow.show();
    return QApplication::exec();
}

// TUI trampoline: defined in src/tui/tuiapp.cpp when BUILD_TUI=1, stub otherwise
#ifndef BUILD_TUI
int runTuiApp(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positional)
{
    Q_UNUSED(app);
    Q_UNUSED(parser);
    Q_UNUSED(positional);
    QTextStream err(stderr);
    err << "TUI not built\n";
    return 1;
}
#endif

// Next-gen trampoline: defined in src/nextgen/nextgenapp.cpp when BUILD_NEXTGEN=1, stub otherwise
#ifndef BUILD_NEXTGEN
int runNextgenApp(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positional)
{
    Q_UNUSED(app);
    Q_UNUSED(parser);
    Q_UNUSED(positional);
    QTextStream err(stderr);
    err << "Next-gen not built\n";
    return 1;
}
#endif
