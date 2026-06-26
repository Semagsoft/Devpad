# Devpad

A C++/Qt6 code editor with QScintilla syntax highlighting and embedded terminal.

## Features

- Multi-tab editor with QScintilla (syntax highlighting, code folding, brace matching, auto-indentation)
- Syntax highlighting for: C/C++, C#, Java, Python, JavaScript, HTML, CSS, XML, SQL, Bash, CMake, Markdown
- Auto-completion with language keywords
- Auto-close brackets and quotes
- Find, Replace, Go To Line, Find Next/Previous
- Find in Files with regex, whole word, and case-sensitive search
- Snippet expansion with tab-stop navigation and predictive auto-completion
- Bookmark lines per tab
- Toggle line/block comments
- Embedded terminal panel (QTermWidget) — docked or as a tab
- Project panel with file tree, file type icons, filter/search, recent folders
- Split view with drag-and-drop tab reordering across panes
- Drag-and-drop file opening
- External file change detection with reload prompt
- Auto-save with backup and recovery prompt
- Session management (restore tabs and project folder on next launch)
- Encoding support (UTF-8/16/32, ISO-8859-1, System) with BOM detection and reopen/save with encoding
- Print and Print Preview with full syntax highlighting
- Remote file download via HTTP/HTTPS/SSH
- External tools integration with configurable commands and shortcuts
- 7 themes: Light, Dark, Nord, Solarized Light, Monokai, Gruvbox Dark, System
- Zoom in/out/reset, fullscreen mode (F11)
- Read-only mode toggle per tab
- Word wrap, whitespace visibility, scroll past end, vertical edge marker
- Configurable tab bar position, close button side, tab display mode
- Status bar with line/column, file type, encoding selector
- Recent files list, recent folders list
- Command-line: open files and folders by passing paths as arguments
- Internationalization: German, Spanish, French translations
- Easter egg: click the Devpad icon in the About dialog

## Requirements

- Qt6 (Core, Gui, Widgets, PrintSupport, Network, Multimedia)
- qtermwidget6
- QScintilla (qt6 variant, e.g. `qscintilla2-qt6`)
- CMake 3.23 or higher
- C++17 compatible compiler

### Installing dependencies

**Debian/Ubuntu:**
```bash
sudo apt install qt6-base-dev libqtermwidget6-1-dev libqscintilla2-qt6-dev cmake g++
```

**Arch Linux:**
```bash
sudo pacman -S qt6-base qtermwidget qscintilla cmake gcc
```

**Fedora:**
```bash
sudo dnf install qt6-qtbase-devel qtermwidget-devel qscintilla-qt6-devel cmake gcc-c++
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Install system-wide (requires root):
```bash
cmake --install build
```

### Building with tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Usage

```bash
./build/Devpad                     # launch empty editor
./build/Devpad file.txt            # open a file
./build/Devpad file1.cpp file2.py  # open multiple files
./build/Devpad /path/to/project    # open a folder in project panel
```

## Configuration

Settings are persisted via QSettings. Location depends on platform:

- **Linux:** `~/.config/Semagsoft/Devpad.conf`
- **Windows:** Registry under `HKEY_CURRENT_USER\Software\Semagsoft\Devpad`
- **macOS:** `~/Library/Preferences/com.Semagsoft.Devpad.plist`

Backups and logs are stored in `~/.local/share/devpad/`.

## License

GNU General Public License v2.0 or later.

SPDX-License-Identifier: GPL-2.0-or-later
