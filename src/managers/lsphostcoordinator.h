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

#ifndef LSPHOSTCOORDINATOR_H
#define LSPHOSTCOORDINATOR_H

#include <QHash>
#include <QObject>
#include <QPointer>
#include <functional>

class CodeEditor;
class PluginManager;
class ReferencesPanel;
class ErrorListPanel;
class TabManager;

namespace lsp
{
class ILspService;
}

class LspEditorBridge;

/**
 * Host-side glue between the editor core and the LSP plugin.
 *
 * The host never sees LSP implementation types. When the LSP plugin is
 * loaded, each open CodeEditor gets an LspEditorBridge (narrowing the
 * editor to the IEditorLspBridge interface) which is handed to the plugin's
 * ILspService. The plugin creates one session per editor and drives the
 * editor through the bridge.
 *
 * Live enable/disable is supported: attachAll() runs when the plugin loads
 * or is enabled; detachAll() tears bridges down and clears the diagnostic
 * surfaces when it is disabled. Editors hold only a QPointer to the session,
 * so a plugin unload silently no-ops every LSP code path.
 */
class LspHostCoordinator : public QObject
{
    Q_OBJECT

public:
    LspHostCoordinator(TabManager* tabManager, ErrorListPanel* errorListPanel, ReferencesPanel* referencesPanel, PluginManager* pluginManager,
                       std::function<void(const QString& filePath, int line, int column)> navigateCallback, QObject* parent = nullptr);
    ~LspHostCoordinator() override;

    /** Attach every currently open editor to the LSP service, if one is loaded. */
    void attachAll();

    /** Attach a single editor. No-op when the LSP plugin is not loaded or the editor is already attached. */
    void attachEditor(CodeEditor* editor);

    /**
     * Re-detect the file's language and (re)activate the session without
     * recreating the bridge. Safe to call at any time (e.g. after a save):
     * an already-active session is left alone.
     */
    void refreshEditor(CodeEditor* editor);

    /** Detach all bridges and clear diagnostic surfaces (call on plugin disable). */
    void detachAll();

    /** The LSP service, or nullptr when the plugin is not loaded. */
    lsp::ILspService* service() const;

private:
    void ensureServiceSignals(lsp::ILspService* svc);
    void scheduleDidOpen(CodeEditor* editor);

    TabManager* m_tabManager = nullptr;
    ErrorListPanel* m_errorListPanel = nullptr;
    ReferencesPanel* m_referencesPanel = nullptr;
    PluginManager* m_pluginManager = nullptr;
    std::function<void(const QString& filePath, int line, int column)> m_navigate;
    QHash<CodeEditor*, QPointer<LspEditorBridge>> m_bridges;
    QHash<CodeEditor*, int> m_didOpenGenerations;
    QMetaObject::Connection m_referencesConnection;
    bool m_serviceSignalsConnected = false;
};

#endif // LSPHOSTCOORDINATOR_H
