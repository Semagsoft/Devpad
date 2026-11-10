/*
 * Devpad - Terminal plugin
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
 * The Terminal plugin adopts the TerminalPanel created by MainWindow (exposed
 * via PluginHost::registerPanel()) and owns its dock lifecycle.
 *
 * Install: the built libterminalplugin.so + terminalplugin.plugin.json go to
 *   ~/.local/share/devpad/plugins/  (Linux/macOS)
 *   or %APPDATA%\devpad\plugins\     (Windows)
 */

#include "plugins/plugininterface.h"

#include <QDockWidget>
#include <QMainWindow>

#include "terminalpanel.h"

class TerminalPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit TerminalPlugin(QObject* parent = nullptr) : QObject(parent) {}
    ~TerminalPlugin() override = default;

    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = QStringLiteral("org.semagsoft.devpad.terminal");
        i.name = QStringLiteral("Terminal");
        i.version = QStringLiteral("1.0.0");
        i.author = QStringLiteral("Semagsoft");
        i.description = QStringLiteral("Embedded terminal panel.");
        i.apiVersion = 1; // must match PluginManager::HostApiVersion
        i.library = QStringLiteral("libterminalplugin.so");
        i.enabled = true;
        return i;
    }

    bool initialize(IPluginHost* host) override
    {
        m_host = host;

        // MainWindow exposes the built-in TerminalPanel via registerPanel().
        // The panel is already docked by MainWindow; the plugin adopts it and
        // owns its dock lifecycle (removing the dock on shutdown()).
        // Note: qobject_cast cannot be used here because TerminalPanel's
        // staticMetaObject exists in both the main executable and this plugin
        // module (different addresses). Use inherits() + static_cast instead,
        // which compares class name strings.
        auto* panel = host->panel(QStringLiteral("terminal"));
        if (!panel || !panel->inherits("TerminalPanel"))
            return false;
        m_terminalPanel = static_cast<TerminalPanel*>(panel);
        return true;
    }

    void shutdown() override
    {
        m_terminalPanel = nullptr;
        m_host = nullptr;
    }

private:
    IPluginHost* m_host = nullptr;
    TerminalPanel* m_terminalPanel = nullptr;
};

// Exported C factory function resolved by the host via QLibrary::resolve().
extern "C" IPlugin* devpadCreatePlugin()
{
    return new TerminalPlugin;
}

#include "terminalplugin.moc"
