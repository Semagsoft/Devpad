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
#ifndef LSPINDICATORS_H
#define LSPINDICATORS_H

namespace lsp
{

// Scintilla indicator IDs used for LSP diagnostics.
// 21 is SnippetEngine::SNIPPET_INDICATOR, 22 is the find-indicator.
// LSP diagnostics start at 23; keep in sync with any new indicators.
// Scintilla reserves 32–35 for IME input composition; do not use them.
constexpr int LSP_INDICATOR_ERROR = 23;
constexpr int LSP_INDICATOR_WARNING = 24;
constexpr int LSP_INDICATOR_INFO = 25;
constexpr int LSP_INDICATOR_HIGHLIGHT = 26;
constexpr int LSP_INDICATOR_SEMANTIC = 27;

// Margin markers for diagnostic severity on line number margin (margin 0 extension)
// Marker IDs 3, 4, 5 are reserved for LSP diagnostics (bookmarks use 0)
constexpr int LSP_MARKER_ERROR = 3;
constexpr int LSP_MARKER_WARNING = 4;
constexpr int LSP_MARKER_INFO = 5;

// Margin for diagnostic markers (margin 0 = line numbers, margin 1 = bookmarks/fold)
constexpr int LSP_DIAG_MARGIN = 2;

} // namespace lsp

#endif // LSPINDICATORS_H
