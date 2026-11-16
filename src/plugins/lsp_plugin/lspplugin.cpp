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

/*
 * LSP plugin — Language Server Protocol support, shipped as a first-party
 * runtime-loaded plugin. Owns the language server manager and all
 * per-editor sessions; host code talks to it through the interfaces in
 * plugins/lspinterface.h.
 */

#include "lspplugin.h"

#include "lsp/lsptypes.h"
#include "lspconnectionhelper.h"
#include "lspeditorsession.h"
#include "lspservermanager.h"

#include <algorithm>
#include <utility>

using namespace lsp;

LspService::LspService(QObject* parent) : ILspService()
{
    Q_UNUSED(parent);

    m_manager = new LspServerManager(this);

    // Broadcast workspace symbol responses for the Find Symbols dialog.
    connect(m_manager, &LspServerManager::workspaceSymbolsReady, this, &LspService::workspaceSymbolsReady);

    // Bridge everything else through the connection helper. The helper is
    // given the live registry of bridges and the service's own navigation
    // callback dispatcher (set by the host via setNavigateCallback).
    setupLspConnections(m_manager, &m_bridges, this,
                        [this](const QString& filePath, int line, int column)
                        {
                            if (m_navigate)
                                m_navigate(filePath, line, column);
                        });
}

LspService::~LspService() = default;

void LspService::setNavigateCallback(std::function<void(const QString& filePath, int line, int column)> callback)
{
    m_navigate = std::move(callback);
}

void LspService::openDocument(const QString& language, const QString& uri, const QString& text)
{
    if (m_manager)
        m_manager->openDocument(language, uri, text);
}

void LspService::changeDocument(const QString& uri, const QString& text)
{
    if (m_manager)
        m_manager->changeDocument(uri, text);
}

void LspService::closeDocument(const QString& uri)
{
    if (m_manager)
        m_manager->closeDocument(uri);
}

void LspService::saveDocument(const QString& uri)
{
    if (m_manager)
        m_manager->saveDocument(uri);
}

ILspEditorSession* LspService::attachEditor(IEditorLspBridge* bridge)
{
    if (!bridge)
        return nullptr;

    // Already attached?
    if (auto it = m_sessions.constFind(bridge); it != m_sessions.constEnd())
        return it.value();

    auto* session = new LspEditorSession(bridge, this);
    session->setServerManager(m_manager);
    bridge->setSession(session);

    m_bridges.append(QPointer<IEditorLspBridge>(bridge));
    m_sessions.insert(bridge, session);

    // When the editor dies, the bridge dies with it; drop the session.
    connect(bridge, &QObject::destroyed, this,
            [this, bridge]()
            {
                m_bridges.erase(std::remove_if(m_bridges.begin(), m_bridges.end(), [](const QPointer<IEditorLspBridge>& b) { return b.isNull(); }),
                                m_bridges.end());
                // The session self-deletes via its own destroyed() connection.
                m_sessions.remove(bridge);
            });

    return session;
}

void LspService::detachEditor(IEditorLspBridge* bridge)
{
    if (!bridge)
        return;

    m_bridges.removeAll(QPointer<IEditorLspBridge>(bridge));
    if (LspEditorSession* session = m_sessions.take(bridge))
    {
        bridge->setSession(nullptr);
        session->deleteLater();
    }
}

void LspService::registerServer(const QString& language, const QString& command, const QStringList& args)
{
    if (m_manager)
        m_manager->registerPluginServer(language, command, args);
}

void LspService::unregisterServer(const QString& language)
{
    if (m_manager)
        m_manager->unregisterPluginServer(language);
}

QStringList LspService::languages() const
{
    return m_manager ? m_manager->languages() : QStringList();
}

void LspService::requestWorkspaceSymbols(const QString& query)
{
    if (!m_manager)
        return;
    for (const QString& lang : m_manager->languages())
    {
        if (auto* client = m_manager->clientForLanguage(lang))
            client->requestWorkspaceSymbols(query);
    }
}

LspPlugin::LspPlugin(QObject* parent) : QObject(parent)
{
}

LspPlugin::~LspPlugin() = default;

PluginInfo LspPlugin::info() const
{
    PluginInfo i;
    i.id = QStringLiteral("org.semagsoft.devpad.lsp");
    i.name = QStringLiteral("Language Server Protocol (LSP)");
    i.version = QStringLiteral("1.0.0");
    i.author = QStringLiteral("Semagsoft");
    i.description = QStringLiteral("Language Server Protocol support: diagnostics, completion, navigation, and more.");
    i.apiVersion = 1; // must match PluginManager::HostApiVersion
    i.library = QStringLiteral("liblsp_plugin" PLUGIN_SUFFIX);
    i.enabled = true;
    return i;
}

bool LspPlugin::initialize(IPluginHost* host)
{
    m_host = host;

    m_service = new LspService(this);
    m_host->registerObject(QString::fromLatin1(LspServiceObjectId), m_service);

    return true;
}

void LspPlugin::shutdown()
{
    // PluginManager calls this before unloading the library. Deleting the
    // sessions first is safe: every host-side holder is a QPointer, which
    // nulls out as the sessions die.
    if (m_service && m_host)
        m_host->unregisterObject(QString::fromLatin1(LspServiceObjectId));

    delete m_service;
    m_service = nullptr;
    m_host = nullptr;
}

extern "C" IPlugin* devpadCreatePlugin()
{
    return new LspPlugin;
}

#include "lspplugin.moc"
