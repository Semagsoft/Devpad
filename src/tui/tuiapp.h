/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * Minimal TUI frontend using ncurses. Headless counterpart to MainWindow.
 */

#ifndef TUIAPP_H
#define TUIAPP_H

#include <QCoreApplication>
#include <QStringList>

class QCommandLineParser;

class TuiApp
{
public:
    // Run TUI event loop. Returns exit code.
    static int run(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positionalFiles);

private:
    TuiApp() = delete;
};

// Free function trampoline called from main.cpp (avoids header dependency on ncurses)
int runTuiApp(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positional);

#endif // TUIAPP_H
