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

#ifndef LSPDEFAULTS_H
#define LSPDEFAULTS_H

// Built-in default language server commands, shared between the LSP plugin
// (which launches the servers) and the host's Options dialog (which shows
// them as defaults). Pure data; no dependencies.

#include <QHash>
#include <QString>
#include <QStringList>

namespace lsp
{

inline QHash<QString, QString> defaultServerCommands()
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
        {"jsonc", "vscode-langservers-extracted"},
        {"cmake", "cmake-language-server"},
    };
}

inline QStringList defaultServerArgs(const QString& language)
{
    if (language == "javascript" || language == "typescript")
        return {"--stdio"};
    if (language == "bash")
        return {"start"};
    if (language == "html" || language == "css" || language == "json" || language == "jsonc")
        return {"--stdio"};
    return {};
}

} // namespace lsp

#endif // LSPDEFAULTS_H
