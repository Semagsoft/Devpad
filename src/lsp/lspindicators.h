#ifndef LSPINDICATORS_H
#define LSPINDICATORS_H

namespace lsp {

// Scintilla indicator IDs used for LSP diagnostics
// INDICATOR 20 = error (squiggle underline, red)
// INDICATOR 21 = warning (straight box/underline, yellow) -- note: 21 used by snippets, but we only use it when LSP is active
// INDICATOR 22 = info/hint (dotted underline, blue) -- note: 22 used by find indicators
// We use 23, 24, 25 for LSP diagnostics to avoid conflicts

constexpr int LSP_INDICATOR_ERROR = 23;
constexpr int LSP_INDICATOR_WARNING = 24;
constexpr int LSP_INDICATOR_INFO = 25;
constexpr int LSP_INDICATOR_HIGHLIGHT = 26;
constexpr int LSP_INDICATOR_SEMANTIC = 27;

// Margin markers for diagnostic severity on line number margin (margin 0 extension)
// We use marker IDs 3, 4, 5 for LSP diagnostics (bookmarks use 0)
constexpr int LSP_MARKER_ERROR = 3;
constexpr int LSP_MARKER_WARNING = 4;
constexpr int LSP_MARKER_INFO = 5;

// Margin number for diagnostic markers (reusing bookmark margin or a new one)
// We use margin 2 for diagnostic markers (margin 0=line numbers, margin 1=bookmarks/fold)
constexpr int LSP_DIAG_MARGIN = 2;

} // namespace lsp

#endif // LSPINDICATORS_H
