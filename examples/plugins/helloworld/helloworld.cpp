/*
 * Devpad - Example "Hello World" plugin
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
 * Build (from this directory):
 *   cmake -B build -DCMAKE_PREFIX_PATH=<path to Qt6> \
 *              -DDEVPAD_SRC_DIR=<path to Devpad/src>
 *   cmake --build build -j
 *
 * Install:
 *   Copy build/libhelloworld.so and helloworld.plugin.json to
 *   ~/.local/share/devpad/plugins/  (Linux/macOS)
 *   or %APPDATA%\devpad\plugins\     (Windows)
 *   Then restart Devpad and look for "Hello World" in the Tools menu.
 */

#include "plugins/plugininterface.h"

#include <QAction>
#include <QApplication>
#include <QMessageBox>

class HelloWorldPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit HelloWorldPlugin(QObject* parent = nullptr) : QObject(parent) {}
    ~HelloWorldPlugin() override = default;

    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = QStringLiteral("org.semagsoft.devpad.helloworld");
        i.name = QStringLiteral("Hello World");
        i.version = QStringLiteral("1.0.0");
        i.author = QStringLiteral("Semagsoft");
        i.description = QStringLiteral("Example plugin demonstrating the Devpad plugin API.");
        i.apiVersion = 1; // must match PluginManager::HostApiVersion
        i.library = QStringLiteral("libhelloworld.so");
        return i;
    }

    bool initialize(IPluginHost* host) override
    {
        m_host = host;
        m_action = host->registerAction(
            QStringLiteral("helloworld.greet"), QStringLiteral("&Hello World"), QKeySequence(),
            [this]() { sayHello(); });
        if (!m_action)
            return false;

        // Add the action to the Tools menu (widget frontend only).
        host->addMenuAction(QStringLiteral("Tools"), m_action);
        return true;
    }

    void shutdown() override
    {
        if (m_host && m_action)
            m_host->unregisterAction(QStringLiteral("helloworld.greet"));
        m_action = nullptr;
        m_host = nullptr;
    }

private Q_SLOTS:
    void sayHello()
    {
        QMessageBox::information(QApplication::activeWindow(),
                                 QStringLiteral("Hello World"),
                                 QStringLiteral("Greetings from the Devpad plugin system!"));
    }

private:
    IPluginHost* m_host = nullptr;
    QAction* m_action = nullptr;
};

// Exported C factory function resolved by the host via QLibrary::resolve().
extern "C" IPlugin* devpadCreatePlugin()
{
    return new HelloWorldPlugin;
}

#include "helloworld.moc"
