/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "nextgenapp.h"

#include "core/fileservice.h"
#include "devpad_version.h"
#include "nextgenactions.h"
#include "nextgentabmodel.h"
#include "primoeditor.h"
#include "primofindinfiles.h"
#include "primoterminal.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

#ifdef BUILD_NEXTGEN
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#endif

int runNextgenApp(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positional)
{
    Q_UNUSED(parser);
#ifdef BUILD_NEXTGEN
    auto* guiApp = qobject_cast<QGuiApplication*>(app);
    if (!guiApp)
    {
        QTextStream err(stderr);
        err << "Next-gen mode requires a GUI application (no display?)\n";
        return 1;
    }

    // Headless / CI fallback: cat files to stdout when no display
    if (qEnvironmentVariableIsSet("QT_QPA_PLATFORM") && qgetenv("QT_QPA_PLATFORM") == "offscreen")
    {
        QTextStream out(stdout);
        if (positional.isEmpty())
        {
            out << "Devpad Nextgen (primoEditor) - offscreen mode, no files.\n";
            return 0;
        }
        for (const QString& arg : positional)
        {
            QString filePath = arg;
            if (!QFileInfo(filePath).isAbsolute())
                filePath = QDir::current().absoluteFilePath(filePath);
            QFileInfo fi(filePath);
            if (fi.isFile())
            {
                FileLoadResult res = FileService::load(filePath);
                out << "=== " << filePath << " ===\n";
                if (res.ok)
                    out << res.text << "\n";
                else
                    out << "Error: " << res.error << "\n";
            }
            else if (fi.isDir())
            {
                out << "=== dir: " << filePath << " ===\n";
            }
            else
            {
                out << "=== (new) " << filePath << " ===\n";
            }
        }
        return 0;
    }

    // Non-TTY fallback (pipes) – also cat
    if (!qEnvironmentVariableIsSet("DISPLAY") && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
    {
        // Try to detect if we can open a window; if not, fallback to cat
        // We still attempt QML; if it fails, QQmlApplicationEngine will error.
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &engine,
                     [](const QList<QQmlError>& warns)
                     {
                         QTextStream err(stderr);
                         for (auto& w : warns)
                             err << "QML warning: " << w.toString() << "\n";
                     });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &engine,
                     [](const QUrl& url)
                     {
                         QTextStream err(stderr);
                         err << "QML objectCreationFailed: " << url.toString() << "\n";
                     });

    // Expose initial files to QML
    QStringList absFiles;
    QString initialFolder;
    for (const QString& arg : positional)
    {
        QString filePath = arg;
        if (!QFileInfo(filePath).isAbsolute())
            filePath = QDir::current().absoluteFilePath(filePath);
        QFileInfo fi(filePath);
        if (fi.isDir() && initialFolder.isEmpty())
            initialFolder = filePath;
        absFiles << filePath;
    }

    engine.rootContext()->setContextProperty(QStringLiteral("initialFiles"), absFiles);
    engine.rootContext()->setContextProperty(QStringLiteral("initialFolder"), initialFolder);
    engine.rootContext()->setContextProperty(QStringLiteral("devpadVersion"), QStringLiteral(DEVPAD_VERSION));

    // Bridge for Menu/Toolbar/StatusBar (persists showToolbar/showStatusbar via SettingsManager)
    auto* actions = new NextgenActions(app);
    engine.rootContext()->setContextProperty(QStringLiteral("nextgenActions"), actions);

    // Tab model for draggable tabs + split – shared session if no files
    auto* tabModel = new NextgenTabModel(app);
    bool hasFiles = false;
    for (auto& f : absFiles)
    {
        QFileInfo fi(f);
        if (fi.isFile() || !fi.exists())
        {
            tabModel->addTab(f);
            hasFiles = true;
        }
        if (fi.isDir())
            hasFiles = true;
    }
    // Shared session: if no files and not --no-session, restore from QSettings (shared with Widgets)
    if (!hasFiles && !parser.isSet(QStringLiteral("no-session")))
    {
        // Use SessionManager to load shared session
        QSettings s(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
        // Find latest Session_* group (like SessionManager::loadSessionData)
        QStringList groups = s.childGroups();
        QStringList files;
        for (auto& g : groups)
            if (g.startsWith(QStringLiteral("Session_")))
            {
                s.beginGroup(g);
                QStringList f = s.value(QStringLiteral("Files")).toStringList();
                s.endGroup();
                files.append(f);
            }
        // Legacy fallback
        if (files.isEmpty())
            files = s.value(QStringLiteral("Session_Files")).toStringList();
        for (auto& f : files)
            if (QFileInfo::exists(f))
                tabModel->addTab(f);
        if (!files.isEmpty() && initialFolder.isEmpty())
        {
            // Try to get project path
            for (auto& g : groups)
                if (g.startsWith(QStringLiteral("Session_")))
                {
                    s.beginGroup(g);
                    QString proj = s.value(QStringLiteral("ProjectPath")).toString();
                    s.endGroup();
                    if (!proj.isEmpty() && QDir(proj).exists())
                    {
                        initialFolder = proj;
                        break;
                    }
                }
        }
    }
    engine.rootContext()->setContextProperty(QStringLiteral("tabModel"), tabModel);
    // Save session on quit (shared)
    QObject::connect(guiApp, &QGuiApplication::aboutToQuit, guiApp,
                     [tabModel, initialFolder]()
                     {
                         QSettings s(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
                         // Use nextgen-specific group to share with Widgets' SessionManager which aggregates all Session_*
                         QString grp = QStringLiteral("Session_%1").arg(QCoreApplication::applicationPid());
                         s.beginGroup(grp);
                         s.setValue(QStringLiteral("Files"), tabModel->tabs());
                         s.setValue(QStringLiteral("ProjectPath"), initialFolder);
                         s.endGroup();
                         s.sync();
                     });

    // Shared terminal (singleton) – simple QML TextArea via QProcess, shared across windows
    auto* term = PrimoTerminal::instance();
    if (!initialFolder.isEmpty())
        term->setCurrentDir(initialFolder);
    else if (!tabModel->tabs().isEmpty())
    {
        QFileInfo fi(tabModel->tabAt(0));
        if (fi.exists())
            term->setCurrentDir(fi.absolutePath());
    }
    engine.rootContext()->setContextProperty(QStringLiteral("terminalInstance"), term);

    // Find-in-Files (respects .gitignore + showHidden via SettingsManager)
    auto* finder = new PrimoFindInFiles(app);
    engine.rootContext()->setContextProperty(QStringLiteral("finderInstance"), finder);

    // Register primoEditor types for QML
    qmlRegisterType<PrimoEditor>("Devpad.Nextgen", 1, 0, "PrimoEditor");
    qmlRegisterType<PrimoDocument>("Devpad.Nextgen", 1, 0, "PrimoDocument");
    qmlRegisterType<NextgenActions>("Devpad.Nextgen", 1, 0, "NextgenActions");
    qmlRegisterType<NextgenTabModel>("Devpad.Nextgen", 1, 0, "NextgenTabModel");
    qmlRegisterType<PrimoTerminal>("Devpad.Nextgen", 1, 0, "PrimoTerminal");
    qmlRegisterType<PrimoFindInFiles>("Devpad.Nextgen", 1, 0, "PrimoFindInFiles");

    // Try embedded QML first (resource), fallback to file system for dev builds
    const QUrl qmlUrl(QStringLiteral("qrc:/qml/nextgen/Main.qml"));
    const QString fsQml = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../qml/nextgen/Main.qml"));
    const QString srcQml = QDir(QStringLiteral(__FILE__)).filePath(QStringLiteral("../../../qml/nextgen/Main.qml"));

    bool loaded = false;
    // Check resource
    if (QFile::exists(QStringLiteral(":/qml/nextgen/Main.qml")))
    {
        engine.load(qmlUrl);
        loaded = !engine.rootObjects().isEmpty();
    }
    if (!loaded && QFile::exists(fsQml))
    {
        engine.load(QUrl::fromLocalFile(fsQml));
        loaded = !engine.rootObjects().isEmpty();
    }
    if (!loaded && QFile::exists(srcQml))
    {
        engine.load(QUrl::fromLocalFile(QFileInfo(srcQml).absoluteFilePath()));
        loaded = !engine.rootObjects().isEmpty();
    }
    if (!loaded)
    {
        // Fallback inline QML (minimal window) if no file found – ensures --nextgen works without installed QML
        const QByteArray inlineQml = R"(
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
ApplicationWindow {
    visible: true
    width: 1024; height: 700
    title: "Devpad Nextgen (primoEditor) " + devpadVersion
    Label {
        anchors.centerIn: parent
        text: "Devpad Nextgen — primoEditor\n\n" +
              (initialFiles.length ? "Files: " + initialFiles.join(", ") : "No files") +
              (initialFolder ? "\nFolder: " + initialFolder : "")
        horizontalAlignment: Text.AlignHCenter
    }
}
)";
        engine.loadData(inlineQml, QUrl(QStringLiteral("qrc:/inline_nextgen.qml")));
        loaded = !engine.rootObjects().isEmpty();
    }

    if (engine.rootObjects().isEmpty())
    {
        QTextStream err(stderr);
        err << "Failed to load Nextgen QML UI. Ensure qml/nextgen/Main.qml exists or rebuild with QML resources.\n";
        return 1;
    }

    return guiApp->exec();
#else
    Q_UNUSED(app);
    Q_UNUSED(positional);
    QTextStream err(stderr);
    err << "Next-gen not built. Reconfigure with -DBUILD_NEXTGEN=ON\n";
    return 1;
#endif
}

#ifndef BUILD_NEXTGEN
// Guarded in main.cpp already, but keep for ODR
#endif
