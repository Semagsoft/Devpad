/*
 * Devpad - DAP Debugger Plugin
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
 * The DAP plugin provides Debug Adapter Protocol debugging support. It creates
 * DapServerManager, DebugPanel, and per-editor DapEditorIntegration instances.
 * It communicates with MainWindow via PluginHost::registerObject() and
 * PluginHost::panel().
 *
 * Install: copy build/libdap_plugin.so + dap_plugin.plugin.json to
 *   ~/.local/share/devpad/plugins/  (Linux/macOS)
 *   or %APDP%\devpad\plugins\     (Windows)
 *   Then restart Devpad.
 */

#include "dapplugin.h"

#include "../../codeeditor.h"
#include "../../managers/actionmanager.h"
#include "../../managers/settingsmanager.h"
#include "../lsp/lsptypes.h"
#include "dapclient.h"
#include "dapeditorintegration.h"
#include "dapservermanager.h"
#include "debugpanel.h"

#include <QDebug>
#include <QDockWidget>
#include <QFileInfo>
#include <QJsonObject>
#include <QMetaObject>
#include <QTabWidget>

DapPlugin::DapPlugin(QObject* parent) : QObject(parent)
{
}

DapPlugin::~DapPlugin()
{
    shutdown();
}

PluginInfo DapPlugin::info() const
{
    PluginInfo i;
    i.id = QStringLiteral("org.semagsoft.devpad.dap");
    i.name = QStringLiteral("Debug Adapter Protocol (DAP)");
    i.version = QStringLiteral("1.0.0");
    i.author = QStringLiteral("Semagsoft");
    i.description = QStringLiteral("Debug Adapter Protocol support for debugging via debug adapters (e.g. lldb-vscode, codelldb, debugpy).");
    i.apiVersion = 1; // must match PluginManager::HostApiVersion
    i.library = QStringLiteral("libdap_plugin.so");
    i.enabled = true;
    return i;
}

bool DapPlugin::initialize(IPluginHost* host)
{
    m_host = host;

    // Share the host's SettingsManager. Plugins link Devpad_editor statically,
    // so without this the plugin would read its own private settings cache
    // instead of the app's.
    if (auto* settings = qobject_cast<SettingsManager*>(host->settingsManager()))
        SettingsManager::setSharedInstance(settings);

    // Create DAP server manager.
    m_dapServerManager = new dap::DapServerManager(this);

    // Register DAP server manager so MainWindow can find it.
    m_host->registerObject("dap_server", m_dapServerManager);

    // Create debug panel and register it.
    m_debugPanel = new DebugPanel(nullptr);
    m_debugPanel->setServerManager(m_dapServerManager);
    m_host->registerPanel("debug", m_debugPanel);

    // Connect DAP server manager signals for session lifecycle.
    connect(m_dapServerManager, &dap::DapServerManager::initializeDone, this, &DapPlugin::onInitializeDone);
    connect(m_dapServerManager, &dap::DapServerManager::clientStopped, this, &DapPlugin::onClientStopped);

    qDebug("DapPlugin: initialized");
    return true;
}

void DapPlugin::shutdown()
{
    // Stop all debug sessions first.
    if (m_dapServerManager)
        m_dapServerManager->stopAllSessions();

    m_debugPanel = nullptr;
    m_dapServerManager = nullptr;
    m_editorIntegrations.clear();
    m_host = nullptr;
}

void DapPlugin::onEditorCreated(CodeEditor* editor)
{
    // Create DAP editor integration for this editor.
    auto* integration = new dap::DapEditorIntegration(editor, this);
    integration->setServerManager(m_dapServerManager);
    if (!m_debugSessionKey.isEmpty())
        integration->setSessionKey(m_debugSessionKey);

    editor->setDapIntegration(integration);
    m_editorIntegrations.append(integration);

    qDebug("DapPlugin: editor created, integration set up");
}

void DapPlugin::startDebugging(const QString& sessionKey, const QString& language, const QString& command, const QStringList& args)
{
    // Stop any existing session first.
    stopDebugging(m_debugSessionKey);

    m_debugSessionKey = sessionKey;
    m_debugLanguage = language;
    m_debugCommand = command;
    m_debugArgs = args;

    // Set session key on all editor integrations.
    for (auto* integration : m_editorIntegrations)
        integration->setSessionKey(sessionKey);

    // Start the DAP session.
    if (m_dapServerManager)
        m_dapServerManager->startSession(sessionKey, language, command, args);

    qDebug("DapPlugin: debugging started for session %s", qPrintable(sessionKey));
}

void DapPlugin::stopDebugging(const QString& sessionKey)
{
    if (m_dapServerManager)
        m_dapServerManager->stopSession(sessionKey);

    // Clear session key on all editor integrations.
    for (auto* integration : m_editorIntegrations)
        integration->setSessionKey(QString());

    m_debugSessionKey.clear();

    // Hide debug panel.
    auto* dock = qobject_cast<QDockWidget*>(m_host->panel("debug"));
    if (dock)
        dock->hide();

    // Update action states.
    auto* actionManager = qobject_cast<::ActionManager*>(m_host->actionManager());
    if (actionManager)
    {
        actionManager->debugPanelAct()->setChecked(false);
        actionManager->startDebuggingAct()->setEnabled(true);
        actionManager->stopDebuggingAct()->setEnabled(false);
    }

    qDebug("DapPlugin: debugging stopped for session %s", qPrintable(sessionKey));
}

void DapPlugin::debugContinue(const QString& sessionKey)
{
    if (auto* client = m_dapServerManager ? m_dapServerManager->clientForSession(sessionKey) : nullptr)
        client->continueExecution(0);
}

void DapPlugin::debugStepOver(const QString& sessionKey)
{
    if (auto* client = m_dapServerManager ? m_dapServerManager->clientForSession(sessionKey) : nullptr)
        client->stepOver(0);
}

void DapPlugin::debugStepInto(const QString& sessionKey)
{
    if (auto* client = m_dapServerManager ? m_dapServerManager->clientForSession(sessionKey) : nullptr)
        client->stepInto(0);
}

void DapPlugin::debugStepOut(const QString& sessionKey)
{
    if (auto* client = m_dapServerManager ? m_dapServerManager->clientForSession(sessionKey) : nullptr)
        client->stepOut(0);
}

void DapPlugin::debugRestart(const QString& /*sessionKey*/)
{
    // Restart with the same parameters.
    if (!m_debugSessionKey.isEmpty())
        startDebugging(m_debugSessionKey, m_debugLanguage, m_debugCommand, m_debugArgs);
}

void DapPlugin::onInitializeDone(const QString& sessionKey, const dap::Capabilities& caps)
{
    Q_UNUSED(caps)

    if (sessionKey != m_debugSessionKey)
        return;

    // Launch the debugged program in the current editor.
    auto* editor = m_editorIntegrations.isEmpty() ? nullptr : m_editorIntegrations.first()->editor();
    if (!editor)
        return;

    QJsonObject launchArgs;
    launchArgs["program"] = editor->fileName();
    launchArgs["stopOnEntry"] = true;
    launchArgs["cwd"] = QFileInfo(editor->fileName()).absolutePath();

    if (auto* client = m_dapServerManager->currentClient())
        client->launch(launchArgs);

    // Show debug panel.
    auto* debugPanel = qobject_cast<DebugPanel*>(m_host->panel("debug"));
    if (debugPanel)
    {
        debugPanel->setSessionKey(m_debugSessionKey);
        debugPanel->show();
        debugPanel->raise();
        debugPanel->updateState(true, tr("Starting..."));
    }

    // Update action states.
    auto* actionManager = qobject_cast<::ActionManager*>(m_host->actionManager());
    if (actionManager)
    {
        actionManager->debugPanelAct()->setChecked(true);
        actionManager->startDebuggingAct()->setEnabled(false);
        actionManager->stopDebuggingAct()->setEnabled(true);
    }

    qDebug("DapPlugin: debugging session initialized");
}

void DapPlugin::onClientStopped(const QString& sessionKey)
{
    Q_UNUSED(sessionKey)
    if (sessionKey == m_debugSessionKey)
    {
        m_debugSessionKey.clear();

        auto* actionManager = qobject_cast<::ActionManager*>(m_host->actionManager());
        if (actionManager)
        {
            actionManager->debugPanelAct()->setChecked(false);
            actionManager->startDebuggingAct()->setEnabled(true);
            actionManager->stopDebuggingAct()->setEnabled(false);
        }
    }
}

// Exported C factory function resolved by the host via QLibrary::resolve().
extern "C" IPlugin* devpadCreatePlugin()
{
    return new DapPlugin;
}

#include "dapplugin.moc"
