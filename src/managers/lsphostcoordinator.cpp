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

#include "lsphostcoordinator.h"

#include "appstrings.h"
#include "codeeditor.h"
#include "errorlistpanel.h"
#include "lsp/lspeditorbridge.h"
#include "managers/settingsmanager.h"
#include "managers/tabmanager.h"
#include "plugins/lspinterface.h"
#include "plugins/pluginhost.h"
#include "plugins/pluginmanager.h"
#include "referencespanel.h"

#include <QTimer>
#include <utility>

LspHostCoordinator::LspHostCoordinator(TabManager* tabManager, ErrorListPanel* errorListPanel, ReferencesPanel* referencesPanel,
                                       PluginManager* pluginManager,
                                       std::function<void(const QString& filePath, int line, int column)> navigateCallback, QObject* parent)
    : QObject(parent), m_tabManager(tabManager), m_errorListPanel(errorListPanel), m_referencesPanel(referencesPanel), m_pluginManager(pluginManager),
      m_navigate(std::move(navigateCallback))
{
}

LspHostCoordinator::~LspHostCoordinator() = default;

lsp::ILspService* LspHostCoordinator::service() const
{
#ifdef BUILD_PLUGINS
    if (!m_pluginManager)
        return nullptr;
    PluginHost* host = m_pluginManager->host();
    if (!host)
        return nullptr;
    return qobject_cast<lsp::ILspService*>(host->object(QString::fromLatin1(lsp::LspServiceObjectId)));
#else
    // Plugin support compiled out: no LSP service exists.
    return nullptr;
#endif
}

void LspHostCoordinator::attachAll()
{
    lsp::ILspService* svc = service();
    if (!svc)
        return;

    ensureServiceSignals(svc);

    for (CodeEditor* editor : m_tabManager->editors())
        attachEditor(editor);
}

void LspHostCoordinator::scheduleDidOpen(CodeEditor* editor)
{
    // Both attachEditor and refreshEditor can re-detect the language of the
    // same editor within a short window; only the latest request may send
    // didOpen, or the server may get a duplicate didOpen (protocol error).
    const int generation = ++m_didOpenGenerations[editor];
    QPointer<CodeEditor> guard(editor);
    QTimer::singleShot(200, this,
                       [this, guard, generation]()
                       {
                           if (!guard)
                               return;
                           if (m_didOpenGenerations.value(guard.data(), 0) != generation)
                               return; // superseded by a newer didOpen request
                           if (guard->lspSession())
                               guard->lspSession()->sendDidOpen();
                       });
}

void LspHostCoordinator::ensureServiceSignals(lsp::ILspService* svc)
{
    if (!svc || m_serviceSignalsConnected)
        return;

    // One-time wiring of the navigation callback and service signals so
    // references land in the references panel.
    svc->setNavigateCallback(m_navigate);
    if (m_referencesPanel && !m_referencesConnection)
        m_referencesConnection = connect(svc, &lsp::ILspService::referencesReady, this,
                                         [this](const QString&, const QList<lsp::Location>& locations) { m_referencesPanel->setReferences(locations); });
    m_serviceSignalsConnected = true;
}

void LspHostCoordinator::attachEditor(CodeEditor* editor)
{
    if (!editor || m_bridges.contains(editor))
        return;

    lsp::ILspService* svc = service();
    if (!svc)
        return;

    ensureServiceSignals(svc);

    const QString filePath = editor->fileName();
    QString syntax;
    if (!filePath.isEmpty() && filePath != Strings::untitled())
        syntax = SettingsManager::instance().syntaxForFile(filePath);

    auto* bridge = new LspEditorBridge(editor, editor);
    bridge->setNavigateCallback(m_navigate);
    QPointer<CodeEditor> guard(editor);
    bridge->setIsCurrentCallback([this, guard]() { return guard && m_tabManager && m_tabManager->currentEditor() == guard.data(); });

    lsp::ILspEditorSession* session = svc->attachEditor(bridge);
    editor->setLspSession(session);
    if (session && !syntax.isEmpty())
    {
        session->setLanguage(syntax);
        scheduleDidOpen(editor);
    }

    m_bridges.insert(editor, bridge);
    connect(editor, &QObject::destroyed, this,
            [this, editor]()
            {
                m_bridges.remove(editor);
                m_didOpenGenerations.remove(editor);
            });
}

void LspHostCoordinator::refreshEditor(CodeEditor* editor)
{
    if (!editor)
        return;

    if (!m_bridges.contains(editor))
    {
        // Not attached yet (e.g. plugin became available after the editor was
        // created): do a full attach, which also detects the language.
        attachEditor(editor);
        return;
    }

    lsp::ILspEditorSession* session = editor->lspSession();
    if (!session || session->isActive())
        return;

    const QString filePath = editor->fileName();
    if (filePath.isEmpty() || filePath == Strings::untitled())
        return;
    const QString syntax = SettingsManager::instance().syntaxForFile(filePath);
    if (syntax.isEmpty())
        return;

    session->setLanguage(syntax);
    scheduleDidOpen(editor);
}

void LspHostCoordinator::detachAll()
{
    // Drop every attached bridge (editors that died already nulled their
    // QPointer entries); the plugin's sessions died with the plugin.
    const auto bridges = std::move(m_bridges); // take ownership of the list; bridges die below
    m_bridges.clear();
    m_didOpenGenerations.clear();
    for (const QPointer<LspEditorBridge>& bridge : bridges)
    {
        if (bridge)
            bridge->deleteLater();
    }
    m_serviceSignalsConnected = false;
    if (m_referencesConnection)
    {
        disconnect(m_referencesConnection);
        m_referencesConnection = QMetaObject::Connection();
    }

    // Clear per-editor diagnostic state and the diagnostic surfaces.
    if (m_tabManager)
    {
        for (CodeEditor* editor : m_tabManager->editors())
            if (editor)
            {
                editor->setLspSession(nullptr);
                editor->clearDiagnostics();
            }
    }
    if (m_errorListPanel)
        m_errorListPanel->clearAll();
    if (m_referencesPanel)
        m_referencesPanel->clear();
}
