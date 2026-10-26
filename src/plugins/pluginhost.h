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

#ifndef PLUGINHOST_H
#define PLUGINHOST_H

#include "plugininterface.h"

#include <QDockWidget>
#include <QKeySequence>
#include <QObject>
#include <QPointer>
#include <QVariant>

class ActionManager;
class CodeEditor;
class EditorController;
class MainWindow;
class QMenu;
class QMenuBar;
class QWidget;
class SettingsManager;
class TabManager;

namespace lsp
{
class LspServerManager;
}

/**
 * Concrete host surface exposed to plugins. Wraps the MainWindow-owned managers
 * and forwards editor/file lifecycle signals so plugins (via PluginManager) can
 * observe them without holding direct pointers to every internal class.
 *
 * PluginHost is a QObject; its signals are connected by PluginManager to forward
 * editor/file events to each loaded plugin's virtual handlers on IPlugin.
 */
class PluginHost : public QObject, public IPluginHost
{
    Q_OBJECT

public:
    explicit PluginHost(MainWindow* mainWindow, QObject* parent = nullptr);
    ~PluginHost() override;

    // ── IPluginHost ──────────────────────────────────────────────
    QObject* mainWindow() const override;
    QObject* actionManager() const override;
    QObject* settingsManager() const override;
    QObject* editorController() const override;
    QObject* tabManager() const override;
    QObject* lspServerManager() const override;

    QAction* registerAction(const QString& id, const QString& text, const QKeySequence& shortcut, std::function<void()> callback) override;
    bool unregisterAction(const QString& id) override;
    QAction* action(const QString& id) const override;

    QMenuBar* menuBar() const override;
    QMenu* menu(const QString& path) const override;
    void addMenuAction(const QString& menuPath, QAction* action) override;

    QString addDockWidget(QWidget* widget, Qt::DockWidgetArea area) override;
    bool removeDockWidget(const QString& dockId) override;

    QVariant pluginSetting(const QString& pluginId, const QString& key, const QVariant& defaultValue = QVariant()) const override;
    void setPluginSetting(const QString& pluginId, const QString& key, const QVariant& value) override;
    void removePluginSetting(const QString& pluginId, const QString& key) override;
    QStringList pluginKeys(const QString& pluginId) const override;

    void registerLspServer(const QString& language, const QString& command, const QStringList& args) override;

    bool isWidgetFrontend() const override;

    // ── Internal helpers ─────────────────────────────────────────
    MainWindow* mainWindowRaw() const
    {
        return m_mainWindow;
    }

    // Forward editor/file events to all loaded plugins (called by PluginManager).
signals:
    void editorCreated(CodeEditor* editor);
    void editorClosed(CodeEditor* editor);
    void currentEditorChanged(CodeEditor* editor);
    void fileOpened(const QString& filePath);
    void fileSaved(const QString& filePath);
    void settingsChanged();

private:
    void forwardEditorSignals();

    MainWindow* m_mainWindow = nullptr;
    QPointer<ActionManager> m_actionManager;
    QPointer<SettingsManager> m_settingsManager;
    QPointer<EditorController> m_editorController;
    QPointer<TabManager> m_tabManager;
    QPointer<lsp::LspServerManager> m_lspServerManager;

    // Plugin-registered actions keyed by id so they can be removed on unload.
    QHash<QString, QAction*> m_pluginActions;

    // Dock widgets keyed by dock id so they can be removed on unload.
    struct DockEntry
    {
        QDockWidget* dock = nullptr;
        QString id;
    };
    QHash<QString, DockEntry> m_pluginDocks;
    int m_dockCounter = 0;
};

#endif // PLUGINHOST_H
