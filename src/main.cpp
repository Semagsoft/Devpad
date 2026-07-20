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
#include "mainwindow.h"
#include "managers/settingsmanager.h"
#include "theme.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <array>
#include <cstdlib>
#include <iostream>

#ifdef Q_OS_UNIX
#include <csignal>
#include <cstring>

#include <execinfo.h>
#include <unistd.h>

static void crashHandler(int sig, siginfo_t* info, void*)
{
    const char* msg = "\n=== CRASH ===\nSignal: ";
    (void)write(STDERR_FILENO, msg, strlen(msg));

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
    (void)write(STDERR_FILENO, sigName, strlen(sigName));

    if (info && sig == SIGSEGV)
    {
        const char* addrMsg = "\nFault address: ";
        (void)write(STDERR_FILENO, addrMsg, strlen(addrMsg));
        std::array<char, 32> addr;
        int len = snprintf(addr.data(), addr.size(), "%p", info->si_addr);
        (void)write(STDERR_FILENO, addr.data(), len);
    }

    (void)write(STDERR_FILENO, "\n\nBacktrace:\n", 12);

    std::array<void*, 64> buffer;
    int frames = backtrace(buffer.data(), static_cast<int>(buffer.size()));
    backtrace_symbols_fd(buffer.data(), frames, STDERR_FILENO);

    (void)write(STDERR_FILENO, "\nRun with: ulimit -c unlimited && gdb ./build/Devpad core\n", 59);

    _Exit(EXIT_FAILURE);
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

    QApplication app(argc, argv);
    app.setOrganizationName("Semagsoft");
    app.setOrganizationDomain("semagsoft.com");
    app.setApplicationName("Devpad");
    app.setApplicationVersion(DEVPAD_VERSION);

    initThemeSystem();

    {
        const auto& ui = SettingsManager::instance().uiSettings();
        if (!ui.uiFontFamily.isEmpty())
            QApplication::setFont(QFont(ui.uiFontFamily, ui.uiFontSize));
    }

    MainWindow mainWindow;

    for (int i = 1; i < argc; ++i)
    {
        QString arg = QString::fromLocal8Bit(argv[i]);

        if (arg == "--transfer" && i + 1 < argc)
        {
            QString transferPath = QString::fromLocal8Bit(argv[++i]);
            mainWindow.openTransferFile(transferPath);
            continue;
        }

        QString filePath = arg;
        if (!QFileInfo(filePath).isAbsolute())
            filePath = QDir::current().absoluteFilePath(filePath);

        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile())
            mainWindow.openFileFromPath(filePath);
        else if (fileInfo.isDir())
            mainWindow.openFolderFromPath(filePath);
    }

    mainWindow.show();
    return app.exec();
}
