/*
 * Devpad - Print plugin
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

#include "plugins/plugininterface.h"
#include "plugins/printinterface.h"
#include "printmanager.h"

class PrintPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit PrintPlugin(QObject* parent = nullptr) : QObject(parent)
    {
    }
    ~PrintPlugin() override = default;

    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = QStringLiteral("org.semagsoft.devpad.print");
        i.name = QStringLiteral("Print");
        i.version = QStringLiteral("1.0.0");
        i.author = QStringLiteral("Semagsoft");
        i.description = QStringLiteral("Printing and print preview with syntax-highlighted output.");
        i.apiVersion = 1;
        i.library = QStringLiteral("libprint_plugin" PLUGIN_SUFFIX);
        i.enabled = true;
        return i;
    }

    bool initialize(IPluginHost* host) override
    {
        m_host = host;
        m_manager = new PrintManager();
        host->registerObject(QStringLiteral(DEVPAD_PRINT_SERVICE_OBJECT), m_manager);
        return true;
    }

    void shutdown() override
    {
        if (m_host)
            m_host->unregisterObject(QStringLiteral(DEVPAD_PRINT_SERVICE_OBJECT));
        delete m_manager;
        m_manager = nullptr;
        m_host = nullptr;
    }

private:
    IPluginHost* m_host = nullptr;
    PrintManager* m_manager = nullptr;
};

extern "C" IPlugin* devpadCreatePlugin()
{
    return new PrintPlugin;
}

#include "printplugin.moc"
