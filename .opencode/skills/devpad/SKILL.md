---
name: devpad
description: >
  A C++17/Qt6 code editor built with QScintilla syntax highlighting
  and QTermWidget embedded terminal. Use for contributing to or
  customizing Devpad's source code, build system, themes, snippets,
  translations, and configuration.
triggers:
  - Edit any file under src/, tests/, CMakeLists.txt, or resources/
  - Build, test, or package Devpad (CMake, Flatpak)
  - Add or modify themes, color schemes, QSS styling
  - Add or modify snippets (JSON format)
  - Add or modify translations (TS files under i18n/)
  - Add new languages, lexers, or syntax highlighting
  - Add new managers, dialogs, panels, or widgets
---

# Devpad Skill

A C++/Qt6 code editor with QScintilla syntax highlighting, QTermWidget embedded terminal, project panel, split views, session management, and 7 built-in themes.

**License:** GPL-2.0-only  
**Copyright:** 2026 Semagsoft

## When This Skill Applies

Use this skill when working with:

- **Source code** in `src/` — any `.cpp` or `.h` file
- **Build system** — `CMakeLists.txt`, Flatpak config (`flatpak/com.semagsoft.Devpad.yml`)
- **Tests** — `tests/*.cpp`, GTest + QtTest framework
- **Themes** — `src/theme.*`, `src/themeapplier.*`, `resources/color-schemes/`, `resources/theme.qss`
- **Snippets** — `src/snippet.*`, `src/managers/snippetmanager.*`, `resources/snippets/default.json`
- **Translations** — `i18n/devpad_*.ts`
- **Resources** — `resources/resources.qrc`, icons, sounds
- **CI** — `.github/workflows/ci.yml`

## Architecture

### Directory Layout

```
Devpad/
├── .clang-format          # LLVM style, Allman braces, 4-space indent, 150 col limit
├── .clang-tidy            # modernize, performance, bugprone checks as errors
├── .editorconfig          # 4-space indent, LF, UTF-8
├── CMakeLists.txt         # CMake 3.23+, C++17, Qt6, qtermwidget6, QScintilla
├── flatpak/
│   └── com.semagsoft.Devpad.yml
├── i18n/
│   ├── devpad_de.ts
│   ├── devpad_es.ts
│   └── devpad_fr.ts
├── resources/
│   ├── resources.qrc
│   ├── theme.qss             # QSS template with %placeholder% tokens
│   ├── color-schemes/        # 6 .colorscheme files
│   ├── icons/                # SVG icons by category
│   ├── snippets/
│   │   └── default.json      # VS Code-format snippet definitions
│   └── sounds/
├── src/
│   ├── main.cpp              # Entry point, CLI args (files, folders, --transfer)
│   ├── mainwindow.h/cpp      # Main window, owns all managers/panels
│   ├── codeeditor.h/cpp      # QsciScintilla subclass: editor core
│   ├── editorcontroller.h/cpp# Save/load/close coordination, backup restore
│   ├── theme.h/cpp           # ThemeId enum, ThemeColors struct, 7 themes
│   ├── themeapplier.h/cpp    # Applies theme: QPalette + QSS template substitution
│   ├── languageinfo.h/cpp    # Language → QsciLexer mapping
│   ├── keywords.h/cpp        # Keyword lists per language for auto-completion
│   ├── encodingutils.h/cpp   # BOM detection, encoding conversion
│   ├── snippet.h/cpp         # Snippet data model + VS Code-format body parser
│   ├── logger.h/cpp          # File-based logging
│   ├── defaultsyntax.h       # Default syntax constant
│   ├── appstrings.h          # Centralized UI string literals
│   ├── managers/
│   │   ├── actionmanager.*       # QAction registry, keyboard shortcuts
│   │   ├── backupmanager.*       # Auto-save backups + crash recovery
│   │   ├── encodingmanager.*     # Populates encoding menu/combo
│   │   ├── externaltoolmanager.* # External tool execution
│   │   ├── filemanager.*         # File read/write with conflict detection
│   │   ├── filewatchermanager.*  # QFileSystemWatcher for external changes
│   │   ├── printmanager.*        # Print + print preview with highlighting
│   │   ├── remotefileservice.*   # HTTP(S) file download
│   │   ├── searchmanager.*       # Find/replace/find-in-files coordination
│   │   ├── sessionmanager.*      # Tab/project persistence across launches
│   │   ├── settingsmanager.*     # QSettings wrapper (singleton, cached)
│   │   ├── snippetmanager.*      # Snippet loading + auto-completion
│   │   └── tabmanager.*          # QTabWidget wrapper, editor tracking
│   ├── dialogs/
│   │   ├── aboutdialog.*         # About dialog
│   │   ├── dialogsettings.h      # QDialog geometry persistence helper
│   │   ├── externaltoolsdialog.* # External tools editor dialog
│   │   ├── finddialog.*          # Find text dialog
│   │   ├── findinfilesdialog.*   # Find in files dialog
│   │   ├── optionsdialog.*       # Settings/options dialog
│   │   └── replacedialog.*       # Find & replace dialog
│   ├── panels/
│   │   ├── projectpanel.*    # File tree sidebar with icons, filter, recent
│   │   ├── terminalpanel.*   # QTermWidget wrapper (docked or tab)
│   │   └── splitview.*       # Split editor views (horizontal/vertical)
│   └── widgets/
│       └── themepreviewwidget.*  # Live theme preview in Options dialog
└── tests/
    ├── main.cpp                 # QApplication + GTest main
    ├── test_appstrings.cpp
    ├── test_backupmanager.cpp
    ├── test_codeeditor.cpp
    ├── test_editorcontroller.cpp
    ├── test_encodingutils.cpp
    ├── test_filemanager.cpp
    ├── test_keywords.cpp
    ├── test_languageinfo.cpp
    ├── test_logger.cpp
    ├── test_mainwindow.cpp
    ├── test_remotefileservice.cpp
    ├── test_searchmanager.cpp
    ├── test_sessionmanager.cpp
    ├── test_settingsmanager.cpp
    ├── test_snippet.cpp
    ├── test_snippetmanager.cpp
    ├── test_tabmanager.cpp
    ├── test_theme.cpp
    └── test_themeapplier.cpp
```

### Dependency Graph

```
main.cpp
  └── MainWindow (QMainWindow)
        ├── EditorController  →  FileManager, TabManager, FileWatcherManager, ActionManager
        ├── ActionManager     →  all QActions, keyboard shortcuts
        ├── SessionManager    →  persist/restore tabs + project folder
        ├── ThemeApplier      →  ThemeColors → QPalette + QSS
        ├── EncodingManager   →  encoding menu/combo
        ├── PrintManager      →  QPrinter + print preview
        ├── SearchManager     →  FindDialog, ReplaceDialog
        ├── ExternalToolManager
        ├── RemoteFileService
        ├── SnippetManager
        ├── FileWatcherManager
        ├── TabManager (QTabWidget)
        │     └── CodeEditor (QsciScintilla)  ←  snippet.h, theme.h
        ├── ProjectPanel (QDockWidget)
        ├── TerminalPanel (QDockWidget / QTabWidget tab)
        └── SplitView
```

All managers are singletons or parented to `MainWindow`. The `EditorController` is the primary coordinator for file I/O. SettingsManager is a true singleton (`SettingsManager::instance()`).

## Build System

### Dependencies

| Package | Purpose |
|---------|---------|
| Qt6 Core, Gui, Widgets | Base framework |
| Qt6 PrintSupport | Print + Print Preview |
| Qt6 Multimedia | Sound effects |
| Qt6 Network | Remote file download |
| Qt6 LinguistTools | TS compilation |
| qtermwidget6 | Embedded terminal |
| QScintilla (qt6) | Syntax highlighting |
| CMake 3.23+ | Build system |
| C++17 compiler | g++ or clang++ |

### Building

```bash
# Debug
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# With tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

# Install system-wide
cmake --install build
```

### Flatpak Build

From `flatpak/com.semagsoft.Devpad.yml`:

```bash
cd flatpak
flatpak-builder --force-clean build-dir com.semagsoft.Devpad.yml
flatpak-builder --run build-dir com.semagsoft.Devpad.yml Devpad
```

The Flatpak manifest:
- Uses `org.kde.Platform` runtime (6.8)
- Builds `qtermwidget` from source (git tag 2.1.0)
- Builds `qscintilla` from source archive (2.14.1)
- Builds Devpad with `-DBUILD_TESTS=OFF`

## Coding Conventions

### Formatting (enforced by `.clang-format`)
- LLVM-based style
- Allman braces (`breakBeforeBraces: Allman`)
- 4-space indent, no tabs
- Column limit: 150
- Pointer/reference: left-aligned (`int* p`, `int& r`)
- Includes: regrouped (local `""` before system `<>`)

### Naming
- **Classes:** PascalCase (`CodeEditor`, `SettingsManager`)
- **Methods/Variables:** camelCase (`currentEditor()`, `m_tabWidget`)
- **Member variables:** `m_` prefix (`m_fileName`, `m_snippetActive`)
- **Constants/Enums:** PascalCase (`ThemeId::Light`, `BOOKMARK_MARGIN`)
- **Signals/Slots:** camelCase (`fileSaved()`, `onTabChanged()`)

### Includes
```cpp
// Own header first
#include "codeeditor.h"

// Then project headers
#include "settingsmanager.h"
#include "theme.h"

// Then Qt headers
#include <QApplication>
#include <QPalette>

// Then system headers
#include <array>
#include <mutex>
```

### Conventions
- `Q_OBJECT` macro in every QObject-derived class
- `override` keyword on all overridden virtual methods
- `explicit` on single-arg constructors
- Forward-declare classes in headers where possible
- Use `Q_DISABLE_COPY` for singleton classes
- `constexpr` over `#define` for constants
- Structured bindings and `const auto&` for iteration

### Clang-Tidy (`.clang-tidy`)
- Enabled: `clang-diagnostic-*`, `clang-analyzer-*`, `modernize-*`, `performance-*`, `readability-*`, `bugprone-*`
- Warnings as errors: `modernize-*`, `performance-*`, `bugprone-*`
- Disabled: trailing return type, identifier length, magic numbers

## Testing

### Framework
- **Google Test** (GTest) with QtTest integration
- System GTest preferred; falls back to FetchContent from GitHub (tag v1.14.0)
- `BUILD_TESTS=ON` enables tests
- Test entry point: `tests/main.cpp` (creates `QApplication`)

### Running Tests
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

### Test Patterns
- File: `tests/test_<Component>.cpp`
- Naming: `TEST(ComponentName, TestCaseName)`
- Pattern: instantiate component, call methods, assert with `EXPECT_EQ`, `EXPECT_TRUE`, etc.
- Each test file tests one component class

### Adding a New Test
1. Create `tests/test_newcomponent.cpp`
2. Add source to `TEST_SOURCES` in `CMakeLists.txt`
3. Implement test cases
4. Run with `ctest`

## Theme System

### Architecture

```
ThemeId (enum)
  │
  ▼
getThemeColors(ThemeId) → ThemeColors
  │
  ├── resolve() → computes 23 derived color fields from 19 core values
  │
  ├── getThemePalette(ThemeColors) → QPalette
  │
  ├── buildStyleSheet(ThemeColors) → QSS string
  │     Reads resources/theme.qss, replaces %placeholder% tokens
  │
  └── ThemeApplier::applyTheme(QMainWindow*)
        Sets QPalette + styleSheet on QApplication + window
```

### ThemeId Enum (src/theme.h)
```cpp
enum class ThemeId {
    Light = 0,
    Dark = 1,
    System = 2,
    Nord = 3,
    SolarizedLight = 4,
    Monokai = 5,
    GruvboxDark = 6
};
```

### ThemeColors Struct (src/theme.h)
- **19 core fields:** background, foreground, surfaceBg/Fg, accent, border, caret, lineHighlight, marginBg/Fg, foldMarginBg/Alt, comment, keyword, string, number, operator_, function, preprocessor
- **23 derived fields** (computed by `resolve()`): toolbarBg/Fg, menuBg/Fg, statusbarBg/Fg, tabBg, tabBgActive, tabFg, tabFgActive, tabBorder, scrollbarBg/Handle/HandleHover, selectionBg/Fg, matchedBraceBg/Fg, dialogBg/Fg, inputBg/Fg, buttonBg/Fg, groupboxBg/Fg, checkboxIndicator, separator

### QSS Template (resources/theme.qss)
Uses `%placeholder%` tokens like `%background%`, `%foreground%`, `%menuBg%`, etc. The `ThemeApplier` reads the file and substitutes each token with the hex color from the resolved `ThemeColors`.

### Color Scheme Files (resources/color-schemes/)
Scintilla-specific color scheme files for editor syntax colors. One per theme:
- `DevpadLight.colorscheme`
- `DevpadDark.colorscheme`
- `DevpadGruvboxDark.colorscheme`
- `DevpadMonokai.colorscheme`
- `DevpadNord.colorscheme`
- `DevpadSolarizedLight.colorscheme`

### Adding a New Theme
1. Add entry to `ThemeId` enum in `src/theme.h`
2. Add color values in `getThemeColors()` in `src/theme.cpp`
3. Add a `.colorscheme` file in `resources/color-schemes/`
4. Register the theme in `themeDisplayName()` and `isThemeDark()` in `src/theme.cpp`
5. (Optional) Add accent color support in `SettingsManager`

## Snippet System

### Format
Snippets are stored in VS Code-compatible JSON format in `resources/snippets/default.json`:

```json
{
  "Snippet Name": {
    "prefix": "trigger",
    "body": ["line 1", "line 2 with ${1:default}"],
    "description": "What this snippet does",
    "scope": "cpp,javascript,python"
  }
}
```

- **prefix:** What the user types to trigger auto-completion
- **body:** Array of lines (joined with `\n` at expansion)
- **scope:** Comma-separated language names where this snippet is available
- **Tab stops:** `${1}`, `${2}`, `${1:defaultValue}`, `$0` (final cursor position)
- **Mirrors:** Same tab stop number in multiple places (e.g., `${1:name}` in header guard)

### Tab Stop System (src/snippet.h)
```cpp
struct Snippet {
    QString name;
    QString prefix;
    QString description;
    QStringList body;
    QStringList scope;

    struct TabStop {
        int number;
        QList<int> positions;  // positions in expanded text
        QString defaultValue;
        int length;
    };

    struct ExpandedSnippet {
        QString text;
        QList<TabStop> tabStops;
    };

    ExpandedSnippet expand() const;
};
```

### Snippet Manager (src/managers/snippetmanager.h)
- Loads `resources/snippets/default.json` at startup
- Loads user snippets from config directory
- Methods: `snippetsForLanguage()`, `snippetsByPrefix()`, `snippetByName()`
- Singleton access via `SnippetManager::instance()`

### CodeEditor Snippet Integration
- `insertSnippet(const Snippet&)` — expands and inserts, enters snippet mode
- Tab/Shift+Tab navigates between tab stops
- Visual indicators using Scintilla indicators (indicator ID 21)
- Auto-completion integration via `registerSnippetAutoCompletion()`

### Adding Snippets
1. Edit `resources/snippets/default.json` or create a new file
2. Define snippet with prefix, body lines, scope
3. Rebuild — snippets are loaded at startup via `SnippetManager::loadBuiltIn()`
4. User snippets are loaded from `~/.local/share/devpad/snippets/`

## i18n / Translation Workflow

### File Layout
- `i18n/devpad_de.ts` — German
- `i18n/devpad_es.ts` — Spanish
- `i18n/devpad_fr.ts` — French

### Build Integration
```cmake
qt_add_translations(Devpad
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE qm_files
)
```
TS files are automatically compiled to `.qm` during build.

### Updating Translations
```bash
# Extract strings from source into TS files
lupdate src/ -ts i18n/devpad_de.ts i18n/devpad_es.ts i18n/devpad_fr.ts

# Translate... (edit TS files manually or with Qt Linguist)

# Compile to QM (done automatically by CMake, or manually):
lrelease i18n/devpad_de.ts i18n/devpad_es.ts i18n/devpad_fr.ts
```

### Adding a New Language
1. Create `i18n/devpad_<lang>.ts`
2. Add to `TS_FILES` in `CMakeLists.txt`
3. Run `lupdate` to populate strings
4. Translate and rebuild

## Key Workflows

### Adding a New Manager
1. Create `src/managers/newmanager.h` and `src/managers/newmanager.cpp`
2. Add source files to `Devpad_lib` in `CMakeLists.txt`
3. Forward-declare in `mainwindow.h`
4. Create instance in `MainWindow::setupUI()` or constructor
5. Connect signals/slots in `MainWindow::connectActions()`

### Adding a New Dialog
1. Create files in `src/dialogs/` (e.g., `newdialog.h`, `newdialog.cpp`)
2. Add to `Devpad_lib` in `CMakeLists.txt`
3. Inherit from `QDialog`
4. Use `dialogsettings.h` for geometry persistence if needed
5. Open from MainWindow slot via action in `ActionManager`

### Adding a New Language
1. Add keywords to `src/keywords.cpp` → `keywordsForLanguage()`
2. Add lexer mapping to `src/languageinfo.cpp` → `createLexer()`
3. Add extension→language mapping in `SettingsManager::extensionMap()`
4. Add file name→language mapping in `SettingsManager::fileNameMap()`

### Adding a New File Type Icon
1. Add SVG to `resources/icons/File/`
2. Update `resources/resources.qrc` if needed
3. Register in `ProjectPanel` or relevant icon provider

### Working with Encoding
- `encodingutils.h/cpp` provides BOM detection (`detectBom`) and encoding conversion
- `FileManager::loadFile()`/`saveFile()` accept optional encoding parameter
- Supported: UTF-8, UTF-16 LE/BE, UTF-32 LE/BE, ISO-8859-1, System
- `EncodingManager` populates the status bar encoding combo

### External File Change Detection
- `FileWatcherManager` wraps `QFileSystemWatcher`
- Signals `fileModifiedExternally(filePath)` to MainWindow
- MainWindow shows reload dialog via `EditorController::promptBackupRestore()`

### Auto-Save & Backup
- `BackupManager` saves to `~/.local/share/devpad/backups/`
- `EditorController::autoSave()` called by `QTimer` (configurable interval)
- On startup, `SessionManager` checks for backups newer than saved files
- `EditorController::promptBackupRestore()` offers restore dialog
