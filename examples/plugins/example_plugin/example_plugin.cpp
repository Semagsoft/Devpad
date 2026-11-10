/*
 * Devpad - Example plugin demonstrating participation hooks
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
 *
 * This plugin demonstrates the Devpad plugin participation hooks:
 *   - registerSnippet() / unregisterSnippet()  : contribute code snippets
 *   - registerExtension()                      : map file extensions to languages
 *   - onFileModifiedExternally()               : react to external file changes
 *
 * Install: copy build/libexample_plugin.so + example_plugin.plugin.json to
 *   ~/.local/share/devpad/plugins/  (Linux/macOS)
 *   or %APDP%\devpad\plugins\     (Windows)
 *   Then restart Devpad.
 */

#include "plugins/plugininterface.h"

#include <QObject>
#include <QStringList>
#include <cstdio>

class ExamplePlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit ExamplePlugin(QObject* parent = nullptr) : QObject(parent) {}
    ~ExamplePlugin() override = default;

    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = QStringLiteral("org.semagsoft.devpad.example");
        i.name = QStringLiteral("Example Plugin");
        i.version = QStringLiteral("1.0.0");
        i.author = QStringLiteral("Semagsoft");
        i.description = QStringLiteral("Demonstrates plugin participation hooks.");
        i.apiVersion = 1; // must match PluginManager::HostApiVersion
        i.library = QStringLiteral("libexample_plugin.so");
        i.enabled = true;
        return i;
    }

    bool initialize(IPluginHost* host) override
    {
        m_host = host;
        fprintf(stderr, "ExamplePlugin: initialize() called\n");

        // Register a C++ snippet.
        QStringList cppBody = {
            "#include <iostream>",
            "",
            "int main() {",
            "    ${1:// code}",
            "    return 0;",
            "}",
        };
        bool ok = m_host->registerSnippet(
            QStringLiteral("example.cpp.main"),
            QStringLiteral("cppmain"),
            cppBody,
            QStringLiteral("Example C++ main function"),
            QStringLiteral("cpp"));
        fprintf(stderr, "ExamplePlugin: registerSnippet(cppmain) = %d\n", ok);
        if (!ok)
            return false;

        // Register a global snippet (available in all languages).
        QStringList todoBody = {
            "${1:// TODO: }${2:description}",
        };
        ok = m_host->registerSnippet(
            QStringLiteral("example.todo"),
            QStringLiteral("todo"),
            todoBody,
            QStringLiteral("Example TODO comment"),
            QString()); // empty scope = global
        fprintf(stderr, "ExamplePlugin: registerSnippet(todo) = %d\n", ok);
        if (!ok)
            return false;

        // Map .example extension to a custom language.
        ok = m_host->registerExtension(QStringLiteral("example"), QStringLiteral("example"));
        fprintf(stderr, "ExamplePlugin: registerExtension(example) = %d\n", ok);
        if (!ok)
            return false;

        fprintf(stderr, "ExamplePlugin: initialized successfully\n");
        return true;
    }

    void shutdown() override
    {
        m_host = nullptr;
    }

    void onFileModifiedExternally(const QString& filePath) override
    {
        fprintf(stderr, "ExamplePlugin: file modified externally: %s\n", qPrintable(filePath));
    }

private:
    IPluginHost* m_host = nullptr;
};

extern "C" IPlugin* devpadCreatePlugin()
{
    return new ExamplePlugin;
}

#include "example_plugin.moc"
