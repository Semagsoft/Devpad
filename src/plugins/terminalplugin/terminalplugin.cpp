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
 * The Terminal plugin creates and owns the TerminalPanel. It instantiates
 * TerminalPanel in initialize(), registers it with PluginHost so MainWindow
 * can locate it, and docks it as a bottom dock widget. On shutdown() it
 * removes the dock and releases the panel.
 *
 * Install: the built libterminalplugin.so + terminalplugin.plugin.json go to
 *   ~/.local/share/devpad/plugins/  (Linux/macOS)
 *   or %APPDATA%\devpad\plugins\     (Windows)
 */

#include "managers/settingsmanager.h"
#include "plugins/plugininterface.h"
#include "terminalpanel.h"

#include <QDockWidget>
#include <QMainWindow>

class TerminalPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit TerminalPlugin(QObject* parent = nullptr) : QObject(parent)
    {
    }
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

        // Share the host's SettingsManager. Plugins link Devpad_editor
        // statically, so without this the panel would read its own private
        // settings cache instead of the app's.
        if (auto* settings = qobject_cast<SettingsManager*>(host->settingsManager()))
            SettingsManager::setSharedInstance(settings);

        // Create the terminal panel. MainWindow does not create one; this
        // plugin is the sole owner of TerminalPanel.
        m_terminalPanel = new TerminalPanel(nullptr);

        // Register with the host so MainWindow can locate it via panel("terminal").
        host->registerPanel(QStringLiteral("terminal"), m_terminalPanel);

        // Dock it as a bottom dock widget. addDockWidget() detects that
        // TerminalPanel is already a QDockWidget and adds it directly.
        m_dockId = host->addDockWidget(m_terminalPanel, Qt::BottomDockWidgetArea);
        return !m_dockId.isEmpty();
    }

    void shutdown() override
    {
        if (m_host && !m_dockId.isEmpty())
            m_host->removeDockWidget(m_dockId);
        m_dockId.clear();
        delete m_terminalPanel;
        m_terminalPanel = nullptr;
        m_host = nullptr;
    }

private:
    IPluginHost* m_host = nullptr;
    TerminalPanel* m_terminalPanel = nullptr;
    QString m_dockId;
};

// Exported C factory function resolved by the host via QLibrary::resolve().
extern "C" IPlugin* devpadCreatePlugin()
{
    return new TerminalPlugin;
}

#include "terminalplugin.moc"
