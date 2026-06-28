#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>

#include <cstdlib>
#include <iostream>

#ifdef Q_OS_UNIX
#include <csignal>
#include <cstring>
#include <unistd.h>

static void crashHandler(int sig, siginfo_t* info, void*)
{
    const char* msg = "\n=== CRASH ===\nSegfault at address: ";
    write(STDERR_FILENO, msg, strlen(msg));

    char addr[32];
    int len = snprintf(addr, sizeof(addr), "%p\n", info ? info->si_addr : nullptr);
    write(STDERR_FILENO, addr, len);

    const char* note = "Crash in function: -- cannot determine (backtrace not available) --\n";
    write(STDERR_FILENO, note, strlen(note));

    write(STDERR_FILENO, "\nPossible stack overflow or corrupted frame pointer.\n"
                         "Please run with: ulimit -c unlimited && gdb ./build/Devpad core\n", 110);

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
    app.setApplicationVersion("1.0.0");

    initThemeSystem();

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
