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
#include "lspservermanager.h"
#include "settingsmanager.h"

#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

namespace lsp {

LspServerManager::LspServerManager(QObject* parent)
    : QObject(parent)
{
}

LspServerManager::~LspServerManager()
{
    for (auto* client : m_clients) {
        client->stopServer();
        delete client;
    }
    m_clients.clear();
}

QString LspServerManager::rootUriForFile(const QString& filePath) const
{
    // Look for project root by checking for common markers
    QDir dir = QFileInfo(filePath).absoluteDir();
    while (!dir.isRoot()) {
        if (dir.exists(".git") || dir.exists(".svn") || dir.exists("CMakeLists.txt") ||
            dir.exists("Cargo.toml") || dir.exists("package.json") || dir.exists("go.mod") ||
            dir.exists("pom.xml") || dir.exists("build.gradle") || dir.exists("Setup.hs")) {
            return uriFromPath(dir.absolutePath());
        }
        if (!dir.cdUp())
            break;
    }
    // Fallback: parent directory of the file
    return uriFromPath(QFileInfo(filePath).absolutePath());
}

LspClient* LspServerManager::clientForLanguage(const QString& language)
{
    if (language.isEmpty() || language == "text")
        return nullptr;

    if (m_clients.contains(language))
        return m_clients[language];

    const auto& s = SettingsManager::instance();
    if (!s.lspEnabled())
        return nullptr;

    // Check if this language has LSP configured
    QString serverCommand = s.lspServerCommand(language);
    if (serverCommand.isEmpty())
        serverCommand = defaultServerCommands().value(language);
    if (serverCommand.isEmpty())
        return nullptr;

    // Verify the server binary exists in PATH
    if (QStandardPaths::findExecutable(serverCommand).isEmpty()) {
#ifdef QT_DEBUG
        qDebug().noquote() << QString("LSP server '%1' not found in PATH for %2").arg(serverCommand, language);
#endif
        return nullptr;
    }

    QStringList serverArgs = s.lspServerArgs(language);
    if (serverArgs.isEmpty())
        serverArgs = defaultServerArgs(language);

    auto* client = new LspClient(language, this);
    m_clients[language] = client;

    // Forward signals
    connect(client, &LspClient::completionReady, this, &LspServerManager::completionReady);
    connect(client, &LspClient::definitionReady, this, &LspServerManager::definitionReady);
    connect(client, &LspClient::referencesReady, this, &LspServerManager::referencesReady);
    connect(client, &LspClient::hoverReady, this, &LspServerManager::hoverReady);
    connect(client, &LspClient::codeActionReady, this, &LspServerManager::codeActionReady);
    connect(client, &LspClient::diagnosticsReady, this, &LspServerManager::diagnosticsReady);
    connect(client, &LspClient::documentSymbolsReady, this, &LspServerManager::documentSymbolsReady);
    connect(client, &LspClient::formattingReady, this, &LspServerManager::formattingReady);
    connect(client, &LspClient::rangeFormattingReady, this, &LspServerManager::rangeFormattingReady);
    connect(client, &LspClient::signatureHelpReady, this, &LspServerManager::signatureHelpReady);
    connect(client, &LspClient::renameReady, this, &LspServerManager::renameReady);
    connect(client, &LspClient::documentHighlightReady, this, &LspServerManager::documentHighlightReady);
    connect(client, &LspClient::typeDefinitionReady, this, &LspServerManager::typeDefinitionReady);
    connect(client, &LspClient::declarationReady, this, &LspServerManager::declarationReady);
    connect(client, &LspClient::workspaceSymbolsReady, this, &LspServerManager::workspaceSymbolsReady);
    connect(client, &LspClient::selectionRangesReady, this, &LspServerManager::selectionRangesReady);
    connect(client, &LspClient::linkedEditingRangeReady, this, &LspServerManager::linkedEditingRangeReady);
    connect(client, &LspClient::callHierarchyReady, this, &LspServerManager::callHierarchyReady);
    connect(client, &LspClient::incomingCallsReady, this, &LspServerManager::incomingCallsReady);
    connect(client, &LspClient::outgoingCallsReady, this, &LspServerManager::outgoingCallsReady);
    connect(client, &LspClient::semanticTokensFullReady, this, &LspServerManager::semanticTokensFullReady);

    return client;
}

LspClient* LspServerManager::clientForUri(const QString& uri)
{
    QString lang = m_documentLanguages.value(uri);
    if (lang.isEmpty())
        return nullptr;
    return clientForLanguage(lang);
}

void LspServerManager::openDocument(const QString& language, const QString& uri, const QString& text)
{
    if (!SettingsManager::instance().lspEnabled())
        return;

    QString rootUri = rootUriForFile(pathFromUri(uri));
    auto* client = clientForLanguage(language);
    if (!client)
        return;

    if (!client->isRunning()) {
        QString command = SettingsManager::instance().lspServerCommand(language);
        if (command.isEmpty())
            command = defaultServerCommands().value(language);
        QStringList args = SettingsManager::instance().lspServerArgs(language);
        if (args.isEmpty())
            args = defaultServerArgs(language);
        client->startServer(command, args, rootUri);
    }

    int version = 1;
    m_documentLanguages[uri] = language;
    m_documentVersions[uri] = version;
    client->openDocument(uri, text, version);
}

void LspServerManager::changeDocument(const QString& uri, const QString& text)
{
    auto* client = clientForUri(uri);
    if (!client)
        return;

    int version = m_documentVersions.value(uri, 0) + 1;
    m_documentVersions[uri] = version;
    client->changeDocument(uri, text, version);
}

void LspServerManager::closeDocument(const QString& uri)
{
    auto* client = clientForUri(uri);
    if (!client)
        return;

    client->closeDocument(uri);
    m_documentLanguages.remove(uri);
    m_documentVersions.remove(uri);
}

void LspServerManager::saveDocument(const QString& uri)
{
    auto* client = clientForUri(uri);
    if (!client)
        return;

    client->saveDocument(uri);
}

bool LspServerManager::hasCapability(const QString& uri, const QString& capability) const
{
    QString lang = m_documentLanguages.value(uri);
    if (lang.isEmpty())
        return false;
    auto* client = m_clients.value(lang);
    if (!client || !client->isRunning())
        return false;

    const auto& caps = client->capabilities();
    if (capability == "completion") return caps.completionProvider;
    if (capability == "definition") return caps.definitionProvider;
    if (capability == "codeAction") return caps.codeActionProvider;
    if (capability == "workspaceSymbol") return caps.workspaceSymbolProvider;
    if (capability == "selectionRange") return caps.selectionRangeProvider;
    if (capability == "linkedEditingRange") return caps.linkedEditingRangeProvider;
    if (capability == "callHierarchy") return caps.callHierarchyProvider;
    if (capability == "semanticTokens") return caps.semanticTokensProvider;
    if (capability == "references") return caps.referencesProvider;
    if (capability == "hover") return caps.hoverProvider;
    if (capability == "documentSymbol") return caps.documentSymbolProvider;
    if (capability == "signatureHelp") return caps.signatureHelpProvider;
    if (capability == "formatting") return caps.formattingProvider;
    if (capability == "typeDefinition") return caps.typeDefinitionProvider;
    if (capability == "declaration") return caps.declarationProvider;
    return false;
}

QHash<QString, QString> LspServerManager::defaultServerCommands()
{
    return {
        {"python", "pylsp"},
        {"cpp", "clangd"},
        {"c", "clangd"},
        {"rust", "rust-analyzer"},
        {"go", "gopls"},
        {"javascript", "typescript-language-server"},
        {"typescript", "typescript-language-server"},
        {"bash", "bash-language-server"},
        {"html", "vscode-langservers-extracted"},
        {"css", "vscode-langservers-extracted"},
        {"json", "vscode-langservers-extracted"},
        {"cmake", "cmake-language-server"},
    };
}

QStringList LspServerManager::defaultServerArgs(const QString& language)
{
    if (language == "javascript" || language == "typescript")
        return {"--stdio"};
    if (language == "bash")
        return {"start"};
    if (language == "html" || language == "css" || language == "json")
        return {"--stdio"};
    return {};
}

} // namespace lsp
