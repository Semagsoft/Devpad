# Devpad

A C++/Qt6 code editor with QScintilla syntax highlighting and embedded terminal.

## Features

- Multi-tab editor with QScintilla (syntax highlighting, code folding, brace matching, auto-indentation)
- Syntax highlighting for: C/C++, C#, Java, Python, JavaScript, HTML, CSS, XML, SQL, Bash, CMake, Markdown
- **LSP integration**: go-to-definition, type definition, declaration, find references, rename, hover, code actions, completion, signature help, document highlights, selection ranges, linked editing, call hierarchy, semantic tokens, formatting
- **Diagnostics panel**: error/warning/info markers in gutter and inline squiggle underlines
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
- Multi-pane tab management
- Drag-and-drop file opening
- External file change detection with reload prompt
- Auto-save with backup and recovery prompt
- Session management (restore tabs and project folder on next launch)
- Encoding support (UTF-8/16/32, ISO-8859-1, System) with BOM detection and reopen/save with encoding
- Print and Print Preview with full syntax highlighting
- Remote file download via HTTP/HTTPS
- External tools integration with configurable commands and shortcuts
- **17 built-in themes**: Light, Dark, System, Nord, Solarized Light, Monokai, Gruvbox Dark, Catppuccin (Mocha, Macchiato, Frappé, Latte), Tokyo Night, Tokyo Night Storm, Dracula, One Dark, Ayu Light, Ayu Dark
- Custom user themes from `~/.config/devpad/themes/*.json`
- Zoom in/out/reset, fullscreen mode (F11)
- Read-only mode toggle per tab
- Word wrap, whitespace visibility, scroll past end, vertical edge marker
- Configurable tab bar position, close button side, tab display mode
- (Windows) Menu bar in titlebar option with a modern in-titlebar menu
- Status bar with line/column, file type, encoding selector
- Configurable UI font separate from editor font
- Recent files list, recent folders list
- Command-line: open files and folders by passing paths as arguments (`--transfer` for single-instance)
- Internationalization: German, Spanish, French translations
- Easter egg: click the Devpad icon in the About dialog

## Requirements

- Qt6 (Core, Gui, Widgets, PrintSupport, Network, Multimedia, Svg)
- qtermwidget6 (not needed on Windows — KodoTerm is fetched automatically)
- QScintilla (qt6 variant, e.g. `qscintilla2-qt6`)
- CMake 3.23 or higher
- C++17 compatible compiler

### Installing dependencies

**Debian/Ubuntu:**
```bash
sudo apt install qt6-base-dev qt6-multimedia-dev qt6-printsupport-dev qt6-svg-dev libqtermwidget6-1-dev libqscintilla2-qt6-dev cmake g++
```

**Arch Linux:**
```bash
sudo pacman -S qt6-base qtermwidget qscintilla cmake gcc
```

**Fedora:**
```bash
sudo dnf install qt6-qtbase-devel qtermwidget-devel qscintilla-qt6-devel cmake gcc-c++
```

**Windows (vcpkg):**
```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && .\bootstrap-vcpkg.bat
.\vcpkg install qt6 qscintilla
cd ..
cmake -B build -DCMAKE_BUILD_TYPE=Release "-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
```

**Windows (MSYS2 UCRT64):**
```bash
pacman -S mingw-w64-ucrt-x86_64-qt6 mingw-w64-ucrt-x86_64-qscintilla \
          mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(getconf _NPROCESSORS_ONLN)
```

**macOS (Homebrew):**
```bash
brew install qt cmake librsvg
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)
```
QScintilla and qtermwidget6 are built automatically from source if not found.

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(getconf _NPROCESSORS_ONLN)
```

Install system-wide (requires root):
```bash
cmake --install build
```

### Building with tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build -j$(getconf _NPROCESSORS_ONLN)
ctest --test-dir build --output-on-failure
```

### Packaging for macOS

```bash
./scripts/package-macos.sh
```

Produces `Devpad-<version>-macos-<arch>.dmg` with a bundled `.app`.

For a distributable, Gatekeeper-clean build, sign with a Developer ID and
notarize (requires an Apple Developer account):

```bash
DEVELOPER_ID="Developer ID Application: Your Name (TEAMID)" \
NOTARIZE=1 \
APPLE_ID="you@example.com" \
APPLE_APP_PASSWORD="xxxx-xxxx-xxxx-xxxx" \
APPLE_TEAM_ID="TEAMID" \
./scripts/package-macos.sh
```

Without `DEVELOPER_ID`, the app is ad-hoc signed (fine for local use).

### Installing on macOS

1. Open the `.dmg` and drag `Devpad.app` into `Applications`.
2. Launch Devpad (from Launchpad, Spotlight, or double-clicking the app).
   - First launch of a locally built or ad-hoc signed app: right-click
     `Devpad.app` in Finder and choose **Open**, then confirm. Or run
     `xattr -dr com.apple.quarantine /Applications/Devpad.app` first.
3. If Devpad does not launch after an update (e.g., macOS opens a Terminal or
   nothing happens), re-register the app with LaunchServices:
   ```bash
   ./scripts/fix-launchservices-macos.sh
   ```

Devpad registers itself as an editor for common text/code files, so you can
open files from Finder with **Open With > Devpad** or by dragging them onto the
Dock icon.

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
