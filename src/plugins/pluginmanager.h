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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugininterface.h"

#include <QHash>
#include <QLibrary>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>

class MainWindow;
class PluginHost;

/**
 * PluginManager discovers, loads, and unloads Devpad plugins.
 *
 * Plugins are user-installed shared libraries (e.g. libfoo.so) placed in the
 * user plugins directory:
 *   - Linux/macOS: ~/.local/share/devpad/plugins/
 *   - Windows:     %APPDATA%\devpad\plugins\
 * Each plugin directory contains the library plus an optional
 * <id>.plugin.json manifest.
 *
 * Plugins are loaded with QLibrary and instantiated through the exported C
 * factory function devpadCreatePlugin(). They do not link against Devpad_lib;
 * they only include the plugin interface headers and link Qt.
 *
 * The manager is owned by MainWindow and lives for the application lifetime.
 */
class PluginManager : public QObject
{
    Q_OBJECT

public:
    explicit PluginManager(MainWindow* mainWindow, QObject* parent = nullptr);
    ~PluginManager() override;

    // The directory plugins are loaded from.
    QString pluginsDir() const;
    void setPluginsDir(const QString& dir);

    // API version the host implements. Plugins whose info().apiVersion does
    // not match are rejected.
    static constexpr int HostApiVersion = 1;

    // ── Discovery / loading ──────────────────────────────────────
    // Scan pluginsDir() and load every valid plugin found. Called once during
    // MainWindow startup (after the UI is built). Only enabled plugins are loaded.
    void loadAll();

    // Load a single plugin library by path.
    bool load(const QString& libraryPath);

    // Unload a single plugin by id.
    bool unload(const QString& pluginId);

    // Unload every loaded plugin (used at shutdown).
    void unloadAll();

    // ── Query ────────────────────────────────────────────────────
    bool isLoaded(const QString& pluginId) const;
    QStringList loadedPluginIds() const;
    PluginInfo pluginInfo(const QString& pluginId) const;
    IPlugin* plugin(const QString& pluginId) const;

    // ── Enabled state ────────────────────────────────────────────
    // Whether a plugin is enabled. Defaults to true when no preference is stored.
    bool isPluginEnabled(const QString& pluginId) const;
    // Enable or disable a plugin. Enabling loads it immediately (if the library
    // can be found); disabling unloads it. The preference is persisted.
    void setPluginEnabled(const QString& pluginId, bool enabled);

    // Discover every plugin in pluginsDir(), whether or not it is currently
    // loaded. Used by the preferences UI to list all available plugins.
    QList<PluginInfo> discoveredPlugins() const;

    PluginHost* host() const
    {
        return m_host.get();
    }

signals:
    void pluginLoaded(const QString& pluginId, const PluginInfo& info);
    void pluginUnloaded(const QString& pluginId);
    void pluginFailed(const QString& pluginId, const QString& reason);
    void pluginEnabledChanged(const QString& pluginId, bool enabled);

private:
    bool loadPluginFileAbsolute(const QString& filePath);
    void unloadPlugin(const QString& pluginId);
    static QString pluginIdFromFileName(const QString& fileName);
    static PluginInfo readPluginManifest(const QString& filePath);
    QString pluginFilePath(const QString& pluginId) const;
    void connectHostEventForwarding();
    // Invoke a plugin entry point with an exception guard so a throwing
    // plugin cannot take the whole application down.
    void safePluginCall(const QString& pluginId, const char* event, const std::function<void(IPlugin*)>& fn);

    MainWindow* m_mainWindow = nullptr;
    std::unique_ptr<PluginHost> m_host;
    QString m_pluginsDir;

    // Enabled state persisted via SettingsManager under Plugins/<id>/Enabled.
    QHash<QString, bool> m_enabled;

    // Manifest default enabled state (used when the user has not expressed a preference).
    QHash<QString, bool> m_defaultEnabled;

    // pluginId -> { loader, plugin instance }
    struct LoadedPlugin
    {
        QLibrary* loader = nullptr;
        IPlugin* plugin = nullptr;
        PluginInfo info;
    };
    QHash<QString, LoadedPlugin> m_loaded;
};

#endif // PLUGINMANAGER_H
