# Changelog

## [1.0.0] - 2026-07-06

### Added
- Initial release of Devpad
- Multi-tab editor with QScintilla syntax highlighting for 20+ languages
- LSP integration: go-to-definition, references, rename, hover, completion, diagnostics, code actions, signature help, document highlights, selection ranges, linked editing, semantic tokens, formatting
- Diagnostics panel with error/warning/info gutter markers and squiggle underlines
- Inline find bar with case-sensitive, whole-word, regex matching and replace
- Find in Files with results list and one-click navigation
- Auto-completion with language keywords and LSP-powered completions
- Auto-close brackets and quotes
- Snippet expansion with tab-stop navigation and predictive auto-completion
- Bookmark lines per tab
- Toggle line/block comments
- Embedded terminal panel (QTermWidget/KodoTerm) — docked or as a tab
- Project panel with file tree, file type icons, filter/search, recent folders, rename/new folder context menu
- Split view with drag-and-drop tab reordering across panes
- Drag-and-drop file opening
- External file change detection with reload prompt
- Auto-save with backup and recovery prompt
- Session management (restore tabs and project folder on next launch)
- Encoding support (UTF-8/16/32, ISO-8859-1, System) with BOM detection and reopen/save with encoding
- Print and Print Preview with full syntax highlighting
- Remote file download via HTTP/HTTPS
- External tools integration with configurable commands and shortcuts
- 17 built-in themes: Light, Dark, System, Nord, Solarized Light, Monokai, Gruvbox Dark, Catppuccin (Mocha, Macchiato, Frappe, Latte), Tokyo Night, Tokyo Night Storm, Dracula, One Dark, Ayu Light, Ayu Dark
- Custom user themes from `~/.config/devpad/themes/*.json`
- Internationalization: German, Spanish, French translations
