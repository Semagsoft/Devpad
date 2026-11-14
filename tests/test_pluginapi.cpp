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

#include "plugins/pluginmanager.h"

#include <gtest/gtest.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QString>

namespace
{
qint64 readApiVersion(const QString& path, bool* ok)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        *ok = false;
        return -1;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
    {
        *ok = false;
        return -1;
    }
    *ok = true;
    return doc.object().value(QStringLiteral("apiVersion")).toInt(-1);
}
} // namespace

TEST(PluginApi, TerminalManifestDeclaresSupportedApiVersion)
{
#ifdef DEVPAD_SOURCE_DIR
    bool ok = false;
    const qint64 apiVersion = readApiVersion(QStringLiteral(DEVPAD_SOURCE_DIR "/src/plugins/terminalplugin/terminalplugin.plugin.json"), &ok);
    ASSERT_TRUE(ok) << "Failed to read terminalplugin.plugin.json";
    EXPECT_EQ(PluginManager::HostApiVersion, static_cast<int>(apiVersion));
#else
    GTEST_SKIP() << "DEVPAD_SOURCE_DIR not defined";
#endif
}

TEST(PluginApi, DapManifestDeclaresSupportedApiVersion)
{
#ifdef DEVPAD_SOURCE_DIR
    bool ok = false;
    const qint64 apiVersion = readApiVersion(QStringLiteral(DEVPAD_SOURCE_DIR "/src/plugins/dap_plugin/dap_plugin.plugin.json"), &ok);
    ASSERT_TRUE(ok) << "Failed to read dap_plugin.plugin.json";
    EXPECT_EQ(PluginManager::HostApiVersion, static_cast<int>(apiVersion));
#else
    GTEST_SKIP() << "DEVPAD_SOURCE_DIR not defined";
#endif
}

TEST(PluginApi, BuiltTerminalPluginReportsSupportedApiVersion)
{
#if defined(DEVPAD_BUILD_DIR) && defined(DEVPAD_BUILD_PLUGINS)
    const QString libraryPath = QStringLiteral(DEVPAD_BUILD_DIR "/plugins/libterminalplugin.so");
    QLibrary library(libraryPath);
    ASSERT_TRUE(library.load()) << "Failed to load " << libraryPath.toStdString() << ": " << library.errorString().toStdString();

    using FactoryFn = IPlugin* (*)();
    auto* factory = reinterpret_cast<FactoryFn>(library.resolve("devpadCreatePlugin"));
    ASSERT_NE(factory, nullptr);

    IPlugin* plugin = factory();
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(PluginManager::HostApiVersion, plugin->info().apiVersion);
    delete plugin;
    library.unload();
#else
    GTEST_SKIP() << "DEVPAD_BUILD_DIR/DEVPAD_BUILD_PLUGINS not defined";
#endif
}
