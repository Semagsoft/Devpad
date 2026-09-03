/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * Next-gen QML/high-perf frontend (primoEditor). Parallel to TUI.
 */

#ifndef NEXTGENAPP_H
#define NEXTGENAPP_H

#include <QCoreApplication>
#include <QStringList>

class QCommandLineParser;

int runNextgenApp(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positional);

#endif // NEXTGENAPP_H
