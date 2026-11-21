#include <cstdio>
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

#include "codeeditor.h"
#include "logger.h"
#include "mainwindow.h"
#include "managers/settingsmanager.h"
#include "pluginhost.h"
#include "pluginmanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>
#include <algorithm>
#include <functional>
#include <utility>

PluginManager::PluginManager(MainWindow* mainWindow, QObject* parent) : QObject(parent), m_mainWindow(mainWindow)
{
    m_host = std::make_unique<PluginHost>(mainWindow, this);

    // Default user plugins directory, matching Devpad's existing data dir
    // (~/.local/share/devpad/ on Linux, used for backups and logs).
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QLatin1String("/.local/share");
    setPluginsDir(base + QLatin1String("/devpad/plugins"));

    // Optional environment override for the plugins directory. Takes
    // precedence over the user and system directories. Used by the test
    // suite to load the plugins from the build tree instead of a possibly
    // stale system-wide installation; also handy for portable installs.
    m_envPluginsDir = qEnvironmentVariable("DEVPAD_PLUGINS_DIR");

    // System plugins directory (shipped with the app, read-only).
#ifdef DEVPAD_SYSTEM_PLUGINS_DIR
    m_systemPluginsDir = QStringLiteral(DEVPAD_SYSTEM_PLUGINS_DIR);
#else
    m_systemPluginsDir = QLatin1String("/usr/local/lib/devpad/plugins");
#endif

    // Forward host editor/file events to every loaded plugin.
    connectHostEventForwarding();
}

PluginManager::~PluginManager()
{
    unloadAll();
}

QString PluginManager::pluginsDir() const
{
    return m_pluginsDir;
}

void PluginManager::setPluginsDir(const QString& dir)
{
    m_pluginsDir = QDir::cleanPath(dir);
}

QString PluginManager::systemPluginsDir() const
{
    return m_systemPluginsDir;
}

QString PluginManager::userPluginsDir() const
{
    return m_pluginsDir;
}

QList<PluginInfo> PluginManager::discoveredPlugins() const
{
    QHash<QString, PluginInfo> byId;

    // Environment override directory (highest priority), the system plugins
    // directory (shipped with the app), then the user plugins directory.
    // User-installed plugins take priority over system ones; the environment
    // override takes priority over both.
    appendDiscoveredFromDir(m_systemPluginsDir, byId);
    appendDiscoveredFromDir(m_pluginsDir, byId);
    appendDiscoveredFromDir(m_envPluginsDir, byId);

    QList<PluginInfo> result = byId.values();
    // QHash iteration order is arbitrary; keep the Plugins settings table
    // stable across launches and machines.
    std::sort(result.begin(), result.end(), [](const PluginInfo& a, const PluginInfo& b) { return a.id < b.id; });
    return result;
}

QList<PluginInfo> PluginManager::orderByDependencies(const QList<PluginInfo>& plugins, QStringList* unresolved)
{
    QHash<QString, PluginInfo> byId;
    for (const auto& info : plugins)
        byId.insert(info.id, info);

    QList<PluginInfo> ordered;
    QSet<QString> visited;
    QSet<QString> inProgress;
    QSet<QString> unresolvedIds;

    std::function<bool(const QString&)> visit = [&](const QString& id) -> bool
    {
        if (visited.contains(id))
            return true;
        if (inProgress.contains(id))
            return false; // cycle
        const auto it = byId.constFind(id);
        if (it == byId.end())
            return false; // missing dependency
        inProgress.insert(id);
        for (const QString& dep : it->depends)
        {
            if (!visit(dep))
            {
                unresolvedIds.insert(id);
                inProgress.remove(id);
                return false;
            }
        }
        inProgress.remove(id);
        visited.insert(id);
        ordered.append(*it);
        return true;
    };

    // Visit in stable id order so the result is deterministic.
    QStringList ids = byId.keys();
    ids.sort();
    for (const QString& id : ids)
        visit(id);

    if (unresolved)
    {
        *unresolved = unresolvedIds.values();
        std::sort(unresolved->begin(), unresolved->end());
    }
    return ordered;
}

QString PluginManager::failureReason(const QString& pluginId) const
{
    return m_failures.value(pluginId);
}

void PluginManager::recordFailure(const QString& pluginId, const QString& reason)
{
    m_failures.insert(pluginId, reason);
    Logger::instance().warning(QStringLiteral("PluginManager: plugin '%1': %2").arg(pluginId, reason));
    emit pluginFailed(pluginId, reason);
}

void PluginManager::appendDiscoveredFromDir(const QString& dirPath, QHash<QString, PluginInfo>& byId) const
{
    QDir dir(dirPath);
    if (dirPath.isEmpty() || !dir.exists())
        return;

#ifdef Q_OS_WIN
    const QStringList filters = {QStringLiteral("*.dll")};
#else
    const QStringList filters = {QStringLiteral("*.so")};
#endif
    dir.setNameFilters(filters);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files))
    {
        PluginInfo pluginInfo = readPluginManifest(info.absoluteFilePath());
        byId.insert(pluginInfo.id, pluginInfo);
    }
}

QString PluginManager::pluginFilePath(const QString& pluginId) const
{
    // Environment override takes priority, then the user directory, then the system one.
    QString path = findPluginFileInDir(m_envPluginsDir, pluginId);
    if (!path.isEmpty())
        return path;
    path = findPluginFileInDir(m_pluginsDir, pluginId);
    if (!path.isEmpty())
        return path;
    return findPluginFileInDir(m_systemPluginsDir, pluginId);
}

QString PluginManager::findPluginFileInDir(const QString& dirPath, const QString& pluginId) const
{
    QDir dir(dirPath);
    if (dirPath.isEmpty() || !dir.exists())
        return QString();

#ifdef Q_OS_WIN
    const QStringList filters = {QStringLiteral("*.dll")};
#else
    const QStringList filters = {QStringLiteral("*.so")};
#endif
    dir.setNameFilters(filters);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files))
    {
        if (readPluginManifest(info.absoluteFilePath()).id == pluginId)
            return info.absoluteFilePath();
    }
    return QString();
}

PluginInfo PluginManager::readPluginManifest(const QString& filePath)
{
    QFileInfo info(filePath);
    QString pluginId = pluginIdFromFileName(info.baseName());

    PluginInfo pluginInfo;
    pluginInfo.id = pluginId;
    pluginInfo.library = info.fileName();

    QFile manifestFile(info.absoluteDir().filePath(pluginId + QStringLiteral(".plugin.json")));
    if (manifestFile.open(QIODevice::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
        manifestFile.close();
        if (doc.isObject())
        {
            QJsonObject obj = doc.object();
            QString manifestId = obj.value(QStringLiteral("id")).toString();
            if (!manifestId.isEmpty())
                pluginInfo.id = manifestId;
            pluginInfo.name = obj.value(QStringLiteral("name")).toString();
            pluginInfo.version = obj.value(QStringLiteral("version")).toString();
            pluginInfo.author = obj.value(QStringLiteral("author")).toString();
            pluginInfo.description = obj.value(QStringLiteral("description")).toString();
            pluginInfo.apiVersion = obj.value(QStringLiteral("apiVersion")).toInt(1);
            pluginInfo.library = obj.value(QStringLiteral("library")).toString(info.fileName());
            pluginInfo.enabled = obj.value(QStringLiteral("enabled")).toBool(true);
            const QJsonArray dependsArray = obj.value(QStringLiteral("depends")).toArray();
            for (const auto& dep : dependsArray)
                pluginInfo.depends.append(dep.toString());
        }
    }
    return pluginInfo;
}

void PluginManager::loadAll()
{
    const QList<PluginInfo> plugins = discoveredPlugins();
    for (const auto& info : plugins)
        m_defaultEnabled[info.id] = info.enabled;

    QStringList unresolved;
    const QList<PluginInfo> ordered = orderByDependencies(plugins, &unresolved);
    for (const QString& id : unresolved)
    {
        m_failures.insert(id, QStringLiteral("unresolved dependencies (missing, disabled, or cyclic)"));
        emit pluginFailed(id, m_failures.value(id));
    }

    QStringList failed = unresolved;
    for (const auto& info : ordered)
    {
        if (failed.contains(info.id) || isLoaded(info.id))
            continue;
        if (!isPluginEnabled(info.id))
        {
            failed.append(info.id);
            continue;
        }
        bool depsOk = true;
        for (const QString& dep : info.depends)
        {
            if (failed.contains(dep) || !isPluginEnabled(dep))
            {
                recordFailure(info.id, QStringLiteral("dependency '%1' is unavailable or disabled").arg(dep));
                failed.append(info.id);
                depsOk = false;
                break;
            }
        }
        if (!depsOk)
            continue;
        const QString path = pluginFilePath(info.id);
        if (path.isEmpty() || !loadPluginFileAbsolute(path))
            failed.append(info.id);
    }
}

void PluginManager::loadPluginsFromDir(const QString& dirPath)
{
    QDir dir(dirPath);
    if (dirPath.isEmpty() || !dir.exists())
        return;

#ifdef Q_OS_WIN
    const QStringList filters = {QStringLiteral("*.dll")};
#else
    const QStringList filters = {QStringLiteral("*.so")};
#endif
    dir.setNameFilters(filters);
    const QFileInfoList files = dir.entryInfoList(QDir::Files);

    for (const QFileInfo& info : files)
    {
        // Use the manifest id (reverse-domain style), not the filename-derived id.
        const PluginInfo manifestInfo = readPluginManifest(info.absoluteFilePath());
        const QString id = manifestInfo.id;
        m_defaultEnabled[id] = manifestInfo.enabled;
        if (isLoaded(id))
            continue;
        if (!isPluginEnabled(id))
            continue;
        loadPluginFileAbsolute(info.absoluteFilePath());
    }
}

bool PluginManager::load(const QString& libraryPath)
{
    return loadPluginFileAbsolute(libraryPath);
}

bool PluginManager::loadPluginFileAbsolute(const QString& filePath)
{
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile())
        return false;

    QString pluginId = pluginIdFromFileName(info.baseName());
    if (isLoaded(pluginId))
        return true;

    // Read manifest next to the library, if present: <pluginId>.plugin.json
    PluginInfo pluginInfo = readPluginManifest(filePath);
    pluginId = pluginInfo.id;

    // API version compatibility check.
    if (pluginInfo.apiVersion != HostApiVersion)
    {
        recordFailure(pluginId, QStringLiteral("api version %1 unsupported (host %2)").arg(pluginInfo.apiVersion).arg(HostApiVersion));
        return false;
    }

    // Instantiate via the exported C factory function. No Q_PLUGIN_METADATA and
    // no link against Devpad_lib is required from the plugin.
    QLibrary* loader = new QLibrary(filePath, this);
    if (!loader->load())
    {
        recordFailure(pluginId, loader->errorString());
        loader->deleteLater();
        return false;
    }

    auto func = reinterpret_cast<DevpadCreatePluginFunc>(loader->resolve("devpadCreatePlugin"));
    IPlugin* plugin = func ? func() : nullptr;

    if (!plugin)
    {
        recordFailure(pluginId, QStringLiteral("failed to instantiate plugin (devpadCreatePlugin not found)"));
        loader->unload();
        loader->deleteLater();
        return false;
    }

    LoadedPlugin entry;
    entry.loader = loader;
    entry.plugin = plugin;
    entry.info = std::move(pluginInfo);
    if (entry.info.id.isEmpty())
        entry.info.id = std::move(pluginId);

    // Prefer the plugin's own reported id: a plugin shipped without its
    // .plugin.json would otherwise be registered under the filename-derived id
    // and load a second copy of itself (e.g. a stale user install shadowing
    // the system shipped one).
    const PluginInfo reportedInfo = plugin->info();
    if (!reportedInfo.id.isEmpty())
        entry.info.id = reportedInfo.id;
    if (m_loaded.contains(entry.info.id))
    {
        Logger::instance().warning(
            QStringLiteral("PluginManager: not loading duplicate plugin '%1' from %2 (already loaded)").arg(entry.info.id, filePath));
        delete plugin;
        loader->unload();
        loader->deleteLater();
        return false;
    }

    // Bracket the initialize() call so anything the plugin registers
    // (actions, docks) is attributed to it, even if initialize throws.
    m_host->beginPluginSession(entry.info.id);

    bool initialized = false;
    try
    {
        initialized = plugin->initialize(m_host.get());
    }
    catch (const std::exception& e)
    {
        Logger::instance().error(
            QStringLiteral("Plugin '%1': uncaught exception in initialize(): %2").arg(entry.info.id, QString::fromUtf8(e.what())));
    }
    catch (...)
    {
        Logger::instance().error(QStringLiteral("Plugin '%1': unknown exception in initialize()").arg(entry.info.id));
    }

    if (!initialized)
    {
        recordFailure(entry.info.id, QStringLiteral("initialize() returned false"));
        safePluginCall(entry.info.id, "shutdown", [](IPlugin* p) { p->shutdown(); });
        m_host->endPluginSession(entry.info.id);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        delete plugin;
        loader->unload();
        loader->deleteLater();
        return false;
    }

    m_loaded.insert(entry.info.id, entry);
    m_failures.remove(entry.info.id);

    Logger::instance().info(QStringLiteral("PluginManager: loaded plugin '%1' v%2 (%3)").arg(entry.info.id, entry.info.version, entry.info.library));

    Q_EMIT pluginLoaded(entry.info.id, entry.info);
    return true;
}

void PluginManager::safePluginCall(const QString& pluginId, const char* event, const std::function<void(IPlugin*)>& fn)
{
    IPlugin* instance = plugin(pluginId);
    if (!instance)
        return;
    try
    {
        fn(instance);
    }
    catch (const std::exception& e)
    {
        Logger::instance().error(
            QStringLiteral("Plugin '%1': uncaught exception in %2: %3").arg(pluginId, QString::fromUtf8(event), QString::fromUtf8(e.what())));
    }
    catch (...)
    {
        Logger::instance().error(QStringLiteral("Plugin '%1': unknown exception in %2").arg(pluginId, QString::fromUtf8(event)));
    }
}

bool PluginManager::unload(const QString& pluginId)
{
    if (!m_loaded.contains(pluginId))
        return false;

    LoadedPlugin entry = m_loaded.take(pluginId);
    if (entry.plugin)
    {
        // Invoke shutdown directly on the captured instance; plugin(pluginId)
        // would now return null because we already took the entry.
        try
        {
            entry.plugin->shutdown();
        }
        catch (const std::exception& e)
        {
            Logger::instance().error(QStringLiteral("Plugin '%1': uncaught exception in shutdown: %2").arg(pluginId, QString::fromUtf8(e.what())));
        }
        catch (...)
        {
            Logger::instance().error(QStringLiteral("Plugin '%1': unknown exception in shutdown").arg(pluginId));
        }
        Logger::instance().info(QStringLiteral("PluginManager: unloaded plugin '%1'").arg(pluginId));
    }

    // Remove the plugin's registered actions/docks while its library is still
    // loaded — triggering an action whose callback lives in an unloaded .so
    // would crash, and deleting dock contents after unload would crash in the
    // widget's destructor. endPluginSession() deletes them synchronously.
    if (m_host)
        m_host->endPluginSession(pluginId);

    // Flush any DeferredDelete events posted by the plugin so none of them
    // land after its code is unmapped.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // The plugin instance itself is factory-allocated in the .so; delete it
    // while the library is still mapped.
    if (entry.plugin)
    {
        delete entry.plugin;
        entry.plugin = nullptr;
    }

    if (entry.loader)
    {
        entry.loader->unload();
        entry.loader->deleteLater();
    }

    Q_EMIT pluginUnloaded(pluginId);
    return true;
}

void PluginManager::unloadAll()
{
    QList<QString> ids = m_loaded.keys();
    for (auto it = ids.rbegin(); it != ids.rend(); ++it)
        unload(*it);
    m_loaded.clear();
}

bool PluginManager::isLoaded(const QString& pluginId) const
{
    return m_loaded.contains(pluginId);
}

QStringList PluginManager::loadedPluginIds() const
{
    return m_loaded.keys();
}

PluginInfo PluginManager::pluginInfo(const QString& pluginId) const
{
    return m_loaded.value(pluginId).info;
}

IPlugin* PluginManager::plugin(const QString& pluginId) const
{
    return m_loaded.value(pluginId).plugin;
}

void PluginManager::connectHostEventForwarding()
{
    connect(m_host.get(), &PluginHost::editorCreated, this,
            [this](CodeEditor* editor)
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onEditorCreated", [editor](IPlugin* p) { p->onEditorCreated(editor); });
            });
    connect(m_host.get(), &PluginHost::editorClosed, this,
            [this](CodeEditor* editor)
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onEditorClosed", [editor](IPlugin* p) { p->onEditorClosed(editor); });
            });
    connect(m_host.get(), &PluginHost::currentEditorChanged, this,
            [this](CodeEditor* editor)
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onCurrentEditorChanged", [editor](IPlugin* p) { p->onCurrentEditorChanged(editor); });
            });
    connect(m_host.get(), &PluginHost::fileOpened, this,
            [this](const QString& path)
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onFileOpened", [path](IPlugin* p) { p->onFileOpened(path); });
            });
    connect(m_host.get(), &PluginHost::fileSaved, this,
            [this](const QString& path)
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onFileSaved", [path](IPlugin* p) { p->onFileSaved(path); });
            });
    connect(m_host.get(), &PluginHost::fileModifiedExternally, this,
            [this](const QString& path)
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onFileModifiedExternally", [path](IPlugin* p) { p->onFileModifiedExternally(path); });
            });
    connect(m_host.get(), &PluginHost::settingsChanged, this,
            [this]()
            {
                for (auto& entry : m_loaded)
                    safePluginCall(entry.info.id, "onSettingsChanged", [](IPlugin* p) { p->onSettingsChanged(); });
            });
}

bool PluginManager::isPluginEnabled(const QString& pluginId) const
{
    if (m_enabled.contains(pluginId))
        return m_enabled.value(pluginId);
    const QVariant persisted = SettingsManager::instance().pluginSetting(pluginId, QStringLiteral("Enabled"));
    if (persisted.isValid())
        return persisted.toBool();
    return m_defaultEnabled.value(pluginId, true);
}

void PluginManager::setPluginEnabled(const QString& pluginId, bool enabled)
{
    // Compare against the effective state (cache, persisted setting, or
    // manifest default) rather than only the cache: enabling a plugin that
    // was persisted as disabled must actually run.
    if (isPluginEnabled(pluginId) == enabled)
        return;

    if (!enabled)
    {
        // Never pull the dependency out from under a plugin that is loaded.
        for (const auto& entry : m_loaded)
        {
            if (entry.info.depends.contains(pluginId))
            {
                recordFailure(pluginId, QStringLiteral("required by loaded plugin '%1'").arg(entry.info.id));
                emit pluginEnabledChanged(pluginId, isPluginEnabled(pluginId));
                return;
            }
        }
        unload(pluginId);
    }
    else
    {
        QHash<QString, PluginInfo> byId;
        for (const auto& info : discoveredPlugins())
            byId.insert(info.id, info);
        QSet<QString> visiting;

        std::function<bool(const QString&)> enableChain = [&](const QString& id) -> bool
        {
            if (isLoaded(id) && isPluginEnabled(id))
                return true;
            if (visiting.contains(id))
            {
                recordFailure(id, QStringLiteral("dependency cycle"));
                return false;
            }
            visiting.insert(id);
            const PluginInfo info = byId.value(id);
            for (const QString& dep : info.depends)
            {
                if (!byId.contains(dep))
                {
                    recordFailure(id, QStringLiteral("missing dependency '%1'").arg(dep));
                    visiting.remove(id);
                    return false;
                }
                if (!enableChain(dep))
                {
                    recordFailure(id, QStringLiteral("dependency '%1' could not be enabled").arg(dep));
                    visiting.remove(id);
                    return false;
                }
            }
            visiting.remove(id);

            m_enabled[id] = true;
            SettingsManager::instance().setPluginSetting(id, QStringLiteral("Enabled"), true);
            const QString path = pluginFilePath(id);
            if (path.isEmpty() || !loadPluginFileAbsolute(path))
            {
                recordFailure(id, QStringLiteral("library not found or failed to load"));
                return false;
            }
            emit pluginEnabledChanged(id, true);
            return true;
        };

        if (!enableChain(pluginId))
            // Let the settings table repopulate from the effective state.
            emit pluginEnabledChanged(pluginId, isPluginEnabled(pluginId));
        return; // enableChain already emitted for the target and its dependencies
    }

    m_enabled[pluginId] = enabled;
    SettingsManager::instance().setPluginSetting(pluginId, QStringLiteral("Enabled"), enabled);
    Q_EMIT pluginEnabledChanged(pluginId, enabled);
}

QString PluginManager::pluginIdFromFileName(const QString& fileName)
{
    QString id = fileName;
    if (id.startsWith(QLatin1String("lib"), Qt::CaseInsensitive))
        id = id.mid(3);
    return id;
}
