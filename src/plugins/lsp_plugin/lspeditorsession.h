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

#ifndef LSPEDITORSESSION_H
#define LSPEDITORSESSION_H

#include "plugins/lspinterface.h"

#include <QList>
#include <QPointer>
#include <QString>
#include <QTimer>

namespace lsp
{

class LspServerManager;
class LspClient;

/**
 * Per-editor LSP session owned by the LSP plugin. Replaces the old
 * LspEditorIntegration: instead of a CodeEditor*, the session talks to the
 * host through the IEditorLspBridge interface. The bridge (and therefore
 * this session) is scoped to one editor.
 */
class LspEditorSession : public ILspEditorSession
{
    Q_OBJECT

public:
    explicit LspEditorSession(IEditorLspBridge* bridge, QObject* parent = nullptr);
    ~LspEditorSession() override = default;

    const QString& language() const override
    {
        return m_lspLanguage;
    }
    bool isActive() const override
    {
        return m_lspActive;
    }
    void setLanguage(const QString& language) override;
    int documentVersion() const override
    {
        return m_docVersion;
    }
    void setDocumentVersion(int v) override
    {
        m_docVersion = v;
    }

    void onCharAdded(int charAdded) override;
    void sendDidOpen() override;
    void sendDidChange() override;
    void triggerCompletion() override;
    void requestHover(int position) override;

    void goToDefinition() override;
    void goToTypeDefinition() override;
    void goToDeclaration() override;
    void findReferences() override;
    void formatSelection() override;
    void requestCodeActions() override;
    void requestRename() override;
    void expandSelection() override;
    void shrinkSelection() override;
    void handleSelectionRanges(const QJsonArray& ranges) override;
    void requestDocumentHighlight() override;
    void startCompletionTimer() override;
    void startDiagnosticsTimer() override;

    void setServerManager(LspServerManager* manager);
    LspServerManager* serverManager() const
    {
        return m_lspManager;
    }

    /** Non-owning pointer to the bridge; null once the editor is destroyed. */
    IEditorLspBridge* bridge() const
    {
        return m_bridge.data();
    }

private:
    QString currentUri() const;
    LspClient* clientForCurrentFile() const;

    QPointer<IEditorLspBridge> m_bridge;
    LspServerManager* m_lspManager = nullptr;
    QString m_lspLanguage;
    bool m_lspActive = false;
    bool m_lspCompletionActive = false;
    int m_docVersion = 0;
    QTimer* m_completionTimer = nullptr;
    QTimer* m_diagnosticsTimer = nullptr;
    QTimer* m_highlightTimer = nullptr;
    int m_lastTriggerChar = 0;
    bool m_lastCharWasTrigger = false;

    // Selection range stack for expand/shrink selection
    QList<Range> m_selectionRangeStack;
    int m_selectionRangeDepth = 0;
};

} // namespace lsp

#endif // LSPEDITORSESSION_H
