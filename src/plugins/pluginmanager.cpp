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

#include "pluginmanager.h"
#include "pluginhost.h"

#include "logger.h"
#include "mainwindow.h"
#include "managers/settingsmanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

PluginManager::PluginManager(MainWindow* mainWindow, QObject* parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    m_host = std::make_unique<PluginHost>(mainWindow, this);

    // Default user plugins directory, matching Devpad's existing data dir
    // (~/.local/share/devpad/ on Linux, used for backups and logs).
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QLatin1String("/.local/share");
    setPluginsDir(base + QLatin1String("/devpad/plugins"));

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

QList<PluginInfo> PluginManager::discoveredPlugins() const
{
    QList<PluginInfo> result;
    QDir dir(m_pluginsDir);
    if (!dir.exists())
        return result;

#ifdef Q_OS_WIN
    const QStringList filters = {QStringLiteral("*.dll")};
#else
    const QStringList filters = {QStringLiteral("*.so")};
#endif
    dir.setNameFilters(filters);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files))
        result.append(readPluginManifest(info.absoluteFilePath()));
    return result;
}

QString PluginManager::pluginFilePath(const QString& pluginId) const
{
    QDir dir(m_pluginsDir);
    if (!dir.exists())
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
            for (const QJsonValue& dep : dependsArray)
                pluginInfo.depends.append(dep.toString());
        }
    }
    return pluginInfo;
}

void PluginManager::loadAll()
{
    QDir dir(m_pluginsDir);
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
            qWarning() << "PluginManager: cannot create plugins directory" << m_pluginsDir;
        return;
    }

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
        QString reason = QStringLiteral("api version %1 unsupported (host %2)")
                             .arg(pluginInfo.apiVersion)
                             .arg(HostApiVersion);
        Logger::instance().warning(
            QStringLiteral("PluginManager: failed to load plugin '%1': %2").arg(pluginId, reason));
        emit pluginFailed(pluginId, reason);
        return false;
    }

    // Instantiate via the exported C factory function. No Q_PLUGIN_METADATA and
    // no link against Devpad_lib is required from the plugin.
    QLibrary* loader = new QLibrary(filePath, this);
    if (!loader->load())
    {
        QString reason = loader->errorString();
        Logger::instance().warning(
            QStringLiteral("PluginManager: failed to load plugin '%1': %2").arg(pluginId, reason));
        emit pluginFailed(pluginId, reason);
        loader->deleteLater();
        return false;
    }

    auto func = reinterpret_cast<DevpadCreatePluginFunc>(loader->resolve("devpadCreatePlugin"));
    IPlugin* plugin = func ? func() : nullptr;

    if (!plugin)
    {
        QString reason = QStringLiteral("failed to instantiate plugin (devpadCreatePlugin not found)");
        Logger::instance().warning(
            QStringLiteral("PluginManager: failed to load plugin '%1': %2").arg(pluginId, reason));
        emit pluginFailed(pluginId, reason);
        loader->unload();
        loader->deleteLater();
        return false;
    }

    if (!plugin->initialize(m_host.get()))
    {
        QString reason = QStringLiteral("initialize() returned false");
        Logger::instance().warning(
            QStringLiteral("PluginManager: failed to load plugin '%1': %2").arg(pluginId, reason));
        emit pluginFailed(pluginId, reason);
        plugin->shutdown();
        loader->unload();
        loader->deleteLater();
        return false;
    }

    LoadedPlugin entry;
    entry.loader = loader;
    entry.plugin = plugin;
    entry.info = pluginInfo;
    if (entry.info.id.isEmpty())
        entry.info.id = pluginId;
    m_loaded.insert(entry.info.id, entry);

    Logger::instance().info(
        QStringLiteral("PluginManager: loaded plugin '%1' v%2 (%3)")
            .arg(entry.info.id, entry.info.version, entry.info.library));

    Q_EMIT pluginLoaded(entry.info.id, entry.info);
    return true;
}

bool PluginManager::unload(const QString& pluginId)
{
    if (!m_loaded.contains(pluginId))
        return false;

    LoadedPlugin entry = m_loaded.take(pluginId);
    if (entry.plugin)
    {
        entry.plugin->shutdown();
        Logger::instance().info(
            QStringLiteral("PluginManager: unloaded plugin '%1'").arg(pluginId));
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
                    entry.plugin->onEditorCreated(editor);
            });
    connect(m_host.get(), &PluginHost::editorClosed, this,
            [this](CodeEditor* editor)
            {
                for (auto& entry : m_loaded)
                    entry.plugin->onEditorClosed(editor);
            });
    connect(m_host.get(), &PluginHost::currentEditorChanged, this,
            [this](CodeEditor* editor)
            {
                for (auto& entry : m_loaded)
                    entry.plugin->onCurrentEditorChanged(editor);
            });
    connect(m_host.get(), &PluginHost::fileOpened, this,
            [this](const QString& path)
            {
                for (auto& entry : m_loaded)
                    entry.plugin->onFileOpened(path);
            });
    connect(m_host.get(), &PluginHost::fileSaved, this,
            [this](const QString& path)
            {
                for (auto& entry : m_loaded)
                    entry.plugin->onFileSaved(path);
            });
    connect(m_host.get(), &PluginHost::settingsChanged, this,
            [this]()
            {
                for (auto& entry : m_loaded)
                    entry.plugin->onSettingsChanged();
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
    if (m_enabled.value(pluginId, m_defaultEnabled.value(pluginId, true)) == enabled)
        return;

    m_enabled[pluginId] = enabled;
    SettingsManager::instance().setPluginSetting(pluginId, QStringLiteral("Enabled"), enabled);

    if (enabled)
    {
        const QString filePath = pluginFilePath(pluginId);
        if (!filePath.isEmpty())
            loadPluginFileAbsolute(filePath);
        else
        {
            Logger::instance().warning(
                QStringLiteral("PluginManager: cannot enable plugin '%1': library not found in %2")
                    .arg(pluginId, m_pluginsDir));
        }
    }
    else
    {
        unload(pluginId);
    }

    Q_EMIT pluginEnabledChanged(pluginId, enabled);
}

QString PluginManager::pluginIdFromFileName(const QString& fileName)
{
    QString id = fileName;
    if (id.startsWith(QLatin1String("lib"), Qt::CaseInsensitive))
        id = id.mid(3);
    return id;
}
