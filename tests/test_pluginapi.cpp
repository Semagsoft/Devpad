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

#include "languageinfo.h"
#include "managers/settingsmanager.h"
#include "plugins/pluginhost.h"
#include "plugins/pluginmanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QSignalSpy>
#include <QString>
#include <array>

#include <Qsci/qscilexer.h>
#include <gtest/gtest.h>

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

void checkLanguagePlugin(const QString& name, const QString& id)
{
#ifdef DEVPAD_SOURCE_DIR
    bool ok = false;
    const QString manifest = QStringLiteral(DEVPAD_SOURCE_DIR "/src/plugins/%1/%1.plugin.json").arg(name);
    const qint64 apiVersion = readApiVersion(manifest, &ok);
    ASSERT_TRUE(ok) << "Failed to read " << manifest.toStdString();
    EXPECT_EQ(PluginManager::HostApiVersion, static_cast<int>(apiVersion));
#endif
#if defined(DEVPAD_BUILD_DIR) && defined(DEVPAD_BUILD_PLUGINS)
    const QString libraryPath = QStringLiteral(DEVPAD_BUILD_DIR "/plugins/lib%1" PLUGIN_SUFFIX).arg(name);
    QLibrary library(libraryPath);
    ASSERT_TRUE(library.load()) << "Failed to load " << libraryPath.toStdString() << ": " << library.errorString().toStdString();

    using FactoryFn = IPlugin* (*)();
    auto* factory = reinterpret_cast<FactoryFn>(library.resolve("devpadCreatePlugin"));
    ASSERT_NE(factory, nullptr);

    IPlugin* plugin = factory();
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(PluginManager::HostApiVersion, plugin->info().apiVersion);
    EXPECT_EQ(id, plugin->info().id);
    delete plugin;
    library.unload();
#endif
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
    const QString libraryPath = QStringLiteral(DEVPAD_BUILD_DIR "/plugins/libterminalplugin" PLUGIN_SUFFIX);
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

TEST(PluginApi, LspManifestDeclaresSupportedApiVersion)
{
#ifdef DEVPAD_SOURCE_DIR
    bool ok = false;
    const qint64 apiVersion = readApiVersion(QStringLiteral(DEVPAD_SOURCE_DIR "/src/plugins/lsp_plugin/lsp_plugin.plugin.json"), &ok);
    ASSERT_TRUE(ok) << "Failed to read lsp_plugin.plugin.json";
    EXPECT_EQ(PluginManager::HostApiVersion, static_cast<int>(apiVersion));
#else
    GTEST_SKIP() << "DEVPAD_SOURCE_DIR not defined";
#endif
}

TEST(PluginApi, BuiltLspPluginReportsSupportedApiVersion)
{
#if defined(DEVPAD_BUILD_DIR) && defined(DEVPAD_BUILD_PLUGINS)
    const QString libraryPath = QStringLiteral(DEVPAD_BUILD_DIR "/plugins/liblsp_plugin" PLUGIN_SUFFIX);
    QLibrary library(libraryPath);
    ASSERT_TRUE(library.load()) << "Failed to load " << libraryPath.toStdString() << ": " << library.errorString().toStdString();

    using FactoryFn = IPlugin* (*)();
    auto* factory = reinterpret_cast<FactoryFn>(library.resolve("devpadCreatePlugin"));
    ASSERT_NE(factory, nullptr);

    IPlugin* plugin = factory();
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(PluginManager::HostApiVersion, plugin->info().apiVersion);
    EXPECT_EQ(QStringLiteral("org.semagsoft.devpad.lsp"), plugin->info().id);
    delete plugin;
    library.unload();
#else
    GTEST_SKIP() << "DEVPAD_BUILD_DIR/DEVPAD_BUILD_PLUGINS not defined";
#endif
}

TEST(PluginApi, LanguagePluginsRegisterAndUnregisterLanguages)
{
#if defined(DEVPAD_BUILD_DIR) && defined(DEVPAD_BUILD_PLUGINS)
    struct Spec
    {
        const char* name;
        const char* id;
        const char* languageId;
        const char* extension;
        const char* lexerClass;
        const char* lineComment;
    };
    const std::array<Spec, 7> specs = {{
        {"rustlanguage", "org.semagsoft.devpad.rustlanguage", "rust", "rs", "QsciLexerCPP", "//"},
        {"golanguage", "org.semagsoft.devpad.golanguage", "go", "go", "QsciLexerCPP", "//"},
        {"typescriptlanguage", "org.semagsoft.devpad.typescriptlanguage", "typescript", "ts", "QsciLexerJavaScript", "//"},
        {"qmllanguage", "org.semagsoft.devpad.qmllanguage", "qml", "qml", "QsciLexerJavaScript", "//"},
        {"phplanguage", "org.semagsoft.devpad.phplanguage", "php", "php", "QsciLexerCPP", "//"},
        {"jsonclanguage", "org.semagsoft.devpad.jsonclanguage", "jsonc", "jsonc", "QsciLexerJSON", "//"},
        {"tomllanguage", "org.semagsoft.devpad.tomllanguage", "toml", "toml", "QsciLexerProperties", "#"},
    }};

    // A host with no MainWindow still owns all the static language/settings
    // registries the language hooks use, so plugins can be exercised without
    // constructing the full UI.
    PluginHost host(nullptr);
    for (const auto& spec : specs)
    {
        const QString name = QString::fromLatin1(spec.name);
        const QString id = QString::fromLatin1(spec.id);
        const QString languageId = QString::fromLatin1(spec.languageId);
        const QString extension = QString::fromLatin1(spec.extension);

        QLibrary library(QStringLiteral(DEVPAD_BUILD_DIR "/plugins/lib%1" PLUGIN_SUFFIX).arg(name));
        ASSERT_TRUE(library.load()) << name.toStdString() << ": " << library.errorString().toStdString();
        auto* factory = reinterpret_cast<IPlugin* (*)()>(library.resolve("devpadCreatePlugin"));
        ASSERT_NE(factory, nullptr);
        IPlugin* plugin = factory();
        ASSERT_NE(plugin, nullptr);

        host.beginPluginSession(id);
        ASSERT_TRUE(plugin->initialize(&host)) << name.toStdString() << ": initialize failed";

        const LanguageInfo* lang = findLanguage(languageId);
        ASSERT_NE(lang, nullptr) << name.toStdString() << ": language not registered";
        EXPECT_EQ(lang->lexerClassName, QString::fromLatin1(spec.lexerClass));
        QsciLexer* lexer = lang->factory(nullptr);
        EXPECT_NE(lexer, nullptr) << name.toStdString() << ": lexer factory returned null";
        delete lexer;
        EXPECT_FALSE(lang->keywords().isEmpty()) << name.toStdString() << ": no keywords registered";
        EXPECT_EQ(SettingsManager::instance().syntaxForExtension(extension), languageId);
        const CommentSyntax* syntax = commentSyntaxForLanguage(languageId);
        ASSERT_NE(syntax, nullptr) << name.toStdString() << ": no comment syntax registered";
        EXPECT_EQ(syntax->lineComment, QString::fromLatin1(spec.lineComment));

        plugin->shutdown();
        host.endPluginSession(id);
        delete plugin;
        library.unload();

        EXPECT_EQ(findLanguage(languageId), nullptr) << name.toStdString() << ": language not unregistered";
        EXPECT_EQ(commentSyntaxForLanguage(languageId), nullptr) << name.toStdString() << ": comment syntax not unregistered";
        EXPECT_TRUE(SettingsManager::instance().syntaxForExtension(extension).isEmpty())
            << name.toStdString() << ": extension mapping not unregistered";
    }
#else
    GTEST_SKIP() << "DEVPAD_BUILD_DIR/DEVPAD_BUILD_PLUGINS not defined";
#endif
}

TEST(PluginApi, LanguagePluginsDeclareSupportedApiVersionAndFactory)
{
    struct Plugin
    {
        const char* name;
        const char* id;
    };
    const std::array<Plugin, 8> plugins = {{
        {"rustlanguage", "org.semagsoft.devpad.rustlanguage"},
        {"golanguage", "org.semagsoft.devpad.golanguage"},
        {"typescriptlanguage", "org.semagsoft.devpad.typescriptlanguage"},
        {"qmllanguage", "org.semagsoft.devpad.qmllanguage"},
        {"phplanguage", "org.semagsoft.devpad.phplanguage"},
        {"jsonclanguage", "org.semagsoft.devpad.jsonclanguage"},
        {"tomllanguage", "org.semagsoft.devpad.tomllanguage"},
        {"print_plugin", "org.semagsoft.devpad.print"},
    }};
    for (const auto& plugin : plugins)
        checkLanguagePlugin(QString::fromLatin1(plugin.name), QString::fromLatin1(plugin.id));
}

namespace
{
constexpr const char* kDepA = "org.semagsoft.devpad.test.depa";
constexpr const char* kDepB = "org.semagsoft.devpad.test.depb";
constexpr const char* kMissingDep = "org.semagsoft.devpad.test.missing";
constexpr const char* kBadApi = "org.semagsoft.devpad.test.badapi";

void resetTestPluginSetting(const char* id)
{
    SettingsManager::instance().setPluginSetting(QString::fromLatin1(id), QStringLiteral("Enabled"), false);
}
} // namespace

class PluginDependencyTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        for (const char* id : {kDepA, kDepB, kMissingDep, kBadApi})
            resetTestPluginSetting(id);
        m_manager = std::make_unique<PluginManager>(nullptr);
#if defined(DEVPAD_TEST_PLUGINS_DIR)
        m_manager->setPluginsDir(QStringLiteral(DEVPAD_TEST_PLUGINS_DIR));
#endif
    }

    void TearDown() override
    {
        m_manager.reset();
        for (const char* id : {kDepA, kDepB, kMissingDep, kBadApi})
            resetTestPluginSetting(id);
    }

    std::unique_ptr<PluginManager> m_manager;
};

TEST_F(PluginDependencyTest, EnablingPluginEnablesAndLoadsDependencies)
{
#if defined(DEVPAD_TEST_PLUGINS_DIR)
    ASSERT_FALSE(m_manager->isLoaded(QString::fromLatin1(kDepA)));
    m_manager->setPluginEnabled(QString::fromLatin1(kDepB), true);

    EXPECT_TRUE(m_manager->isPluginEnabled(QString::fromLatin1(kDepA)));
    EXPECT_TRUE(m_manager->isLoaded(QString::fromLatin1(kDepA)));
    EXPECT_TRUE(m_manager->isLoaded(QString::fromLatin1(kDepB)));
#else
    GTEST_SKIP() << "Test plugins not built";
#endif
}

TEST_F(PluginDependencyTest, CannotDisablePluginRequiredByLoadedPlugin)
{
#if defined(DEVPAD_TEST_PLUGINS_DIR)
    m_manager->setPluginEnabled(QString::fromLatin1(kDepB), true);
    ASSERT_TRUE(m_manager->isLoaded(QString::fromLatin1(kDepA)));
    ASSERT_TRUE(m_manager->isLoaded(QString::fromLatin1(kDepB)));

    m_manager->setPluginEnabled(QString::fromLatin1(kDepA), false);

    EXPECT_TRUE(m_manager->isPluginEnabled(QString::fromLatin1(kDepA)));
    EXPECT_TRUE(m_manager->isLoaded(QString::fromLatin1(kDepA)));
    EXPECT_FALSE(m_manager->failureReason(QString::fromLatin1(kDepA)).isEmpty());
#else
    GTEST_SKIP() << "Test plugins not built";
#endif
}

TEST_F(PluginDependencyTest, MissingDependencyFailsWithReasonAndSignal)
{
#if defined(DEVPAD_TEST_PLUGINS_DIR)
    QSignalSpy failed(m_manager.get(), &PluginManager::pluginFailed);
    m_manager->setPluginEnabled(QString::fromLatin1(kMissingDep), true);

    EXPECT_FALSE(m_manager->isLoaded(QString::fromLatin1(kMissingDep)));
    EXPECT_TRUE(m_manager->failureReason(QString::fromLatin1(kMissingDep)).contains(QStringLiteral("missing dependency")));
    EXPECT_EQ(failed.count(), 1);
#else
    GTEST_SKIP() << "Test plugins not built";
#endif
}

TEST_F(PluginDependencyTest, UnsupportedApiVersionRecordsFailure)
{
#if defined(DEVPAD_TEST_PLUGINS_DIR)
    QSignalSpy failed(m_manager.get(), &PluginManager::pluginFailed);
    m_manager->setPluginEnabled(QString::fromLatin1(kBadApi), true);

    EXPECT_FALSE(m_manager->isLoaded(QString::fromLatin1(kBadApi)));
    EXPECT_TRUE(m_manager->failureReason(QString::fromLatin1(kBadApi)).contains(QStringLiteral("api version")));
    EXPECT_EQ(failed.count(), 1);
#else
    GTEST_SKIP() << "Test plugins not built";
#endif
}

TEST(PluginApi, DependencyOrderingLoadsDependenciesFirst)
{
    PluginInfo a;
    a.id = QStringLiteral("a");
    PluginInfo b;
    b.id = QStringLiteral("b");
    b.depends = {a.id};
    PluginInfo c;
    c.id = QStringLiteral("c");
    c.depends = {b.id};

    QStringList unresolved;
    const QList<PluginInfo> ordered = PluginManager::orderByDependencies({c, b, a}, &unresolved);
    ASSERT_EQ(ordered.size(), 3);
    EXPECT_EQ(ordered[0].id, a.id);
    EXPECT_EQ(ordered[1].id, b.id);
    EXPECT_EQ(ordered[2].id, c.id);
    EXPECT_TRUE(unresolved.isEmpty());
}

TEST(PluginApi, DependencyOrderingRejectsMissingDependenciesAndCycles)
{
    PluginInfo missing;
    missing.id = QStringLiteral("missing_dep");
    missing.depends = {QStringLiteral("not_installed")};
    PluginInfo b;
    b.id = QStringLiteral("b");
    b.depends = {QStringLiteral("c")};
    PluginInfo c;
    c.id = QStringLiteral("c");
    c.depends = {b.id};

    QStringList unresolved;
    const QList<PluginInfo> ordered = PluginManager::orderByDependencies({missing, b, c}, &unresolved);
    EXPECT_TRUE(ordered.isEmpty());
    EXPECT_EQ(unresolved.size(), 3);
    EXPECT_TRUE(unresolved.contains(missing.id));
    EXPECT_TRUE(unresolved.contains(b.id));
    EXPECT_TRUE(unresolved.contains(c.id));
}
