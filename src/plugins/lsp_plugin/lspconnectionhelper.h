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

#ifndef LSPCONNECTIONHELPER_H
#define LSPCONNECTIONHELPER_H

#include <QList>
#include <QPointer>
#include <QString>
#include <functional>

namespace lsp
{

class LspServerManager;
class IEditorLspBridge;
class ILspService;
struct Location;

/**
 * Wire the LSP server manager's signals to the editor bridges known to the
 * plugin. bridges is the plugin's live registry of attached editor bridges
 * (non-owning); entries may vanish at any time and are checked for liveness
 * at dispatch.
 *
 * navigateToLocation receives (filePath, 0-based line, UTF-16 column) and is
 * provided by the host. references are published through the service's
 * referencesReady signal; diagnostics are applied to editors via their
 * bridges (host panels observe the editors' own diagnosticsChanged signal).
 */
void setupLspConnections(LspServerManager* manager, const QList<QPointer<IEditorLspBridge>>* bridges, ILspService* service,
                         // line: 0-based. column: LSP UTF-16 code unit column.
                         std::function<void(const QString& filePath, int line, int column)> navigateToLocation);

} // namespace lsp

#endif // LSPCONNECTIONHELPER_H
