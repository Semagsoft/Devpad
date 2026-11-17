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

#ifndef LSPINTERFACE_H
#define LSPINTERFACE_H

/**
 * Public plugin API for the LSP (Language Server Protocol) plugin.
 *
 * The LSP implementation ships as a first-party, runtime-loaded plugin
 * (src/plugins/lsp_plugin). Host code (CodeEditor, MainWindow, ...) must not
 * reference any LSP implementation class; it talks to the plugin exclusively
 * through the interfaces declared here. Both sides include this header;
 * everything else is plugin-private.
 *
 * Wiring:
 *   1. MainWindow creates one IEditorLspBridge per editor (host-side adapter,
 *      src/lsp/lspeditorbridge.h) and hands it to ILspService::attachEditor().
 *   2. The plugin creates an ILspEditorSession per bridge. The editor stores
 *      the session as a QPointer so LSP silently disables itself when the
 *      plugin is unloaded.
 *   3. Editors hold no other LSP implementation types.
 *
 * Column conventions: LSP positions use UTF-16 code units; QScintilla APIs
 * use Unicode code point indices. Conversion helpers live in lsp/lsptypes.h
 * (public DTO header) and are available to both sides.
 */

#include "lsp/lsptypes.h"

#include <QJsonArray>
#include <QList>
#include <QObject>
#include <QString>
#include <functional>

// Scintilla indicator IDs used for LSP diagnostics.
// 21 is SnippetEngine::SNIPPET_INDICATOR, 22 is the find-indicator.
// LSP diagnostics start at 23; keep in sync with any new indicators.
// Scintilla reserves 32–35 for IME input composition; do not use them.
namespace lsp
{
constexpr int LSP_INDICATOR_ERROR = 23;
constexpr int LSP_INDICATOR_WARNING = 24;
constexpr int LSP_INDICATOR_INFO = 25;
constexpr int LSP_INDICATOR_HIGHLIGHT = 26;
constexpr int LSP_INDICATOR_SEMANTIC = 27;

// Marker IDs 3, 4, 5 are reserved for LSP diagnostics (bookmarks use 0).
constexpr int LSP_MARKER_ERROR = 3;
constexpr int LSP_MARKER_WARNING = 4;
constexpr int LSP_MARKER_INFO = 5;

// Margin for diagnostic markers (margin 0 = line numbers, margin 1 = bookmarks/fold)
constexpr int LSP_DIAG_MARGIN = 2;
} // namespace lsp

namespace lsp
{

class ILspEditorSession;

/**
 * Host-implemented view of a single editor. The LSP plugin drives the editor
 * exclusively through this interface; it never sees CodeEditor.
 *
 * The bridge is a QObject so both sides can use QPointer for safe lifetime
 * tracking: when the editor is destroyed, the bridge dies with it and the
 * plugin drops the corresponding session.
 */
class IEditorLspBridge : public QObject
{
    Q_OBJECT

public:
    explicit IEditorLspBridge(QObject* parent = nullptr) : QObject(parent)
    {
    }
    ~IEditorLspBridge() override = default;

    // Document access. fileName() returns QString() for untitled documents.
    virtual QString fileName() const = 0;
    virtual QString documentText() const = 0;
    virtual QString lineText(int line) const = 0;

    // Cursor / selection. Line/column are QScintilla line / Unicode code
    // point column, unless noted otherwise.
    virtual void getCursorPosition(int* line, int* column) const = 0;
    virtual void getSelection(int* lineFrom, int* columnFrom, int* lineTo, int* columnTo) const = 0;
    virtual void setSelection(int lineFrom, int columnFrom, int lineTo, int columnTo) = 0;
    virtual int cursorPosition() const = 0;
    virtual void lineIndexFromPosition(int position, int* line, int* column) const = 0;
    /** Number of consecutive word characters ([A-Za-z0-9_]) immediately before the cursor. */
    virtual int wordCharCountBeforeCursor() const = 0;

    // Diagnostics / highlights / formatting / semantic tokens.
    virtual void applyDiagnostics(const QString& uri, const QList<Diagnostic>& diagnostics) = 0;
    virtual void applyHighlights(const QJsonArray& highlights) = 0;
    virtual void applyFormattingEdits(const QList<QJsonObject>& edits) = 0;
    virtual void applyTextEdits(const QList<QJsonObject>& edits) = 0;
    virtual void applySemanticTokens(const QString& uri, const QJsonArray& tokens) = 0;
    virtual void setLinkedEditingRanges(const QJsonObject& result) = 0;

    // Completion / signature help / hover UI.
    virtual void showCompletion(const CompletionList& completions) = 0;
    virtual void showSignatureHelp(const QJsonObject& info) = 0;
    virtual void showToolTip(int position, const QString& text) = 0;

    /**
     * Navigate the UI to a location. line and column are raw LSP values
     * (0-based line, UTF-16 code unit column); the host converts.
     */
    virtual void navigateToLocation(const QString& filePath, int line, int column) = 0;

    /**
     * The session the plugin attached for this bridge, or nullptr before
     * ILspService::attachEditor() returns / after detach. Both sides treat
     * the pointer as non-owning; the plugin owns the session.
     */
    virtual ILspEditorSession* session() const = 0;
    virtual void setSession(ILspEditorSession* session) = 0;

    /** True when this editor is the one the user is looking at. */
    virtual bool isCurrentEditor() const = 0;
};

/**
 * Plugin-implemented per-editor LSP session. Host code holds a QPointer to
 * this; when the plugin unloads, all sessions are destroyed and the QPointers
 * null out, silently disabling every LSP call site.
 */
class ILspEditorSession : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~ILspEditorSession() override = default;

    virtual const QString& language() const = 0;
    virtual bool isActive() const = 0;
    virtual void setLanguage(const QString& language) = 0;
    virtual int documentVersion() const = 0;
    virtual void setDocumentVersion(int version) = 0;

    virtual void onCharAdded(int charAdded) = 0;
    virtual void sendDidOpen() = 0;
    virtual void sendDidChange() = 0;
    virtual void triggerCompletion() = 0;
    virtual void requestHover(int position) = 0;

    virtual void goToDefinition() = 0;
    virtual void goToTypeDefinition() = 0;
    virtual void goToDeclaration() = 0;
    virtual void findReferences() = 0;
    virtual void formatSelection() = 0;
    virtual void requestCodeActions() = 0;
    virtual void requestRename() = 0;
    virtual void expandSelection() = 0;
    virtual void shrinkSelection() = 0;
    virtual void handleSelectionRanges(const QJsonArray& ranges) = 0;
    virtual void requestDocumentHighlight() = 0;
    virtual void startCompletionTimer() = 0;
    virtual void startDiagnosticsTimer() = 0;
};

/**
 * Plugin-implemented LSP service. Registered via PluginHost::registerObject()
 * under object id "lsp_service" while the plugin is loaded.
 *
 * Host code looks it up with:
 *   qobject_cast<lsp::ILspService*>(host->object("lsp_service"))
 * and must handle nullptr (plugin disabled/absent) on every call site.
 */
class ILspService : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~ILspService() override = default;

    virtual void setNavigateCallback(std::function<void(const QString& filePath, int line, int column)> callback) = 0;

    // Route document lifecycle events for URIs that don't map to an attached
    // bridge (e.g. close events for untitled/removed tabs).
    virtual void openDocument(const QString& language, const QString& uri, const QString& text) = 0;
    virtual void changeDocument(const QString& uri, const QString& text) = 0;
    virtual void closeDocument(const QString& uri) = 0;
    virtual void saveDocument(const QString& uri) = 0;

    // Create (or return the existing) session for an editor bridge. Passing
    // the same bridge twice returns the same session. Passing a bridge whose
    // file is empty/untitled returns a session with isActive() == false.
    virtual ILspEditorSession* attachEditor(IEditorLspBridge* bridge) = 0;
    virtual void detachEditor(IEditorLspBridge* bridge) = 0;

    // Registers (or replaces) an external language server command for a
    // language. Takes precedence over user settings and built-in defaults.
    virtual void registerServer(const QString& language, const QString& command, const QStringList& args) = 0;
    virtual void unregisterServer(const QString& language) = 0;

    virtual QStringList languages() const = 0;
    virtual void requestWorkspaceSymbols(const QString& query) = 0;

signals:
    void workspaceSymbolsReady(const QString& query, const QJsonArray& symbols);
    void referencesReady(const QString& uri, const QList<Location>& locations);
};

/** Object id under which the plugin registers its ILspService. */
constexpr const char* LspServiceObjectId = "lsp_service";

} // namespace lsp

#endif // LSPINTERFACE_H
