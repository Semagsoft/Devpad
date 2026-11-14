cmake_minimum_required(VERSION 3.23)
project(Devpad VERSION 1.10 LANGUAGES CXX)

set(DEPS_PREFIX ${CMAKE_BINARY_DIR}/_deps)
option(BUILD_QSCINTILLA "Build QScintilla from source if not found system-wide" ON)
option(BUILD_TUI "Build terminal UI (ncurses) mode" OFF)
option(BUILD_NEXTGEN "Build next-gen QML/high-perf mode (primoEditor)" OFF)
option(BUILD_PLUGINS "Build the C++ plugin/extension system" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core Gui Widgets PrintSupport Multimedia Network Svg LinguistTools
)
# ── Optional D-Bus: freedesktop portal color-scheme query (Qt 6.2+ backport
# of the QStyleHints::colorScheme portal check that requires Qt 6.5). Keeps
# system-dark detection working on the shipped Qt 6.4.2. Gracefully absent
# on platforms without Qt6 DBus (Windows/macOS minimal installs).
find_package(Qt6 6.2 COMPONENTS DBus QUIET)
if(TARGET Qt6::DBus)
    message(STATUS "Qt6 DBus found; enabling portal-based dark-mode detection")
else()
    message(STATUS "Qt6 DBus not found; portal-based dark-mode detection disabled")
endif()
# ── Terminal backend: libghostty-vt (default) ─────────────────────
option(BUILD_GHOSTTY "Build the libghostty-vt terminal backend via FetchContent (Zig)" ON)
if(BUILD_GHOSTTY)
    include(FetchContent)
    message(STATUS "Fetching libghostty-vt (ghostty) via FetchContent...")
    FetchContent_Declare(ghostty
        GIT_REPOSITORY https://github.com/ghostty-org/ghostty.git
        GIT_TAG vt-1.0.0
    )
    FetchContent_MakeAvailable(ghostty)
    set(TERMINAL_TARGET ghostty-vt-static)
    set(TERMINAL_HAS_GHOSTTY TRUE)
    set(DEVPAD_GHOSTTY_TERMINAL TRUE)
endif()
if(NOT DEFINED TERMINAL_HAS_GHOSTTY)
    set(TERMINAL_HAS_GHOSTTY FALSE)
endif()

# ── Terminal widget (legacy backends; only used when libghostty is off) ──
if(NOT TERMINAL_HAS_GHOSTTY)
if(WIN32)
    include(FetchContent)
    message(STATUS "Fetching KodoTerm terminal widget (Windows)...")
    FetchContent_Declare(KodoTerm
        GIT_REPOSITORY https://github.com/diegoiast/KodoTerm.git
        # Pinned to the tip of main as of 2026-11-12; do not float on a
        # moving branch. Bump deliberately and re-test the Windows build.
        GIT_TAG 9d016815076a451ad639fa021d46602585e91247
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(KodoTerm)
    set(TERMINAL_TARGET KodoTerm::KodoTerm)
    set(TERMINAL_HAS_KODOTERM TRUE)
else()
    find_package(qtermwidget6 QUIET)
    if(NOT qtermwidget6_FOUND)
        message(STATUS "qtermwidget6 not found; building from source")
        include(ExternalProject)

        ExternalProject_Add(ep_lxqt_build_tools
            GIT_REPOSITORY https://github.com/lxqt/lxqt-build-tools.git
            GIT_TAG 2.1.0
            PREFIX ${DEPS_PREFIX}/lxqt-build-tools
            PATCH_COMMAND sh -c "sed -i.bak 's/set(LXQT_MIN_LINGUIST_VERSION \"6.6\")/set(LXQT_MIN_LINGUIST_VERSION \"6.2\")/' CMakeLists.txt && rm -f CMakeLists.txt.bak"
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
        )

        ExternalProject_Add(ep_qtermwidget
            GIT_REPOSITORY https://github.com/lxqt/qtermwidget.git
            GIT_TAG 2.1.0
            PREFIX ${DEPS_PREFIX}/qtermwidget
            PATCH_COMMAND sh -c "sed -i.bak 's/ SHARED / STATIC /' CMakeLists.txt && sed -i.bak 's/set(QT_MINIMUM_VERSION \"6.6.0\")/set(QT_MINIMUM_VERSION \"6.2.0\")/' CMakeLists.txt && rm -f CMakeLists.txt.bak"
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX}
                -DCMAKE_PREFIX_PATH=${DEPS_PREFIX};${CMAKE_PREFIX_PATH}
                -DBUILD_SHARED_LIBS=OFF
            DEPENDS ep_lxqt_build_tools
        )

        add_library(qtermwidget6 STATIC IMPORTED)
        file(MAKE_DIRECTORY ${DEPS_PREFIX}/include ${DEPS_PREFIX}/include/qtermwidget6)
        set_target_properties(qtermwidget6 PROPERTIES
            IMPORTED_LOCATION ${DEPS_PREFIX}/lib/libqtermwidget6.a
            INTERFACE_INCLUDE_DIRECTORIES "${DEPS_PREFIX}/include;${DEPS_PREFIX}/include/qtermwidget6"
            INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui;Qt6::Widgets"
        )
        add_dependencies(qtermwidget6 ep_qtermwidget)

        list(PREPEND CMAKE_PREFIX_PATH ${DEPS_PREFIX})
    endif()
    set(TERMINAL_TARGET qtermwidget6)
endif()
endif()

# ── QScintilla (system or build from source) ──────────────────
# On MSYS2/MinGW the toolchain prefix (e.g. /ucrt64) is not part of the
# default search path, so find_library()/find_path() cannot locate packages
# installed under it (QScintilla ships no CMake config there). Derive the
# prefix from the compiler location and add it to the search path.
if(WIN32 AND MINGW)
    get_filename_component(_mingw_prefix "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_mingw_prefix "${_mingw_prefix}" DIRECTORY)
    list(APPEND CMAKE_PREFIX_PATH "${_mingw_prefix}")
endif()

find_package(Qt6 COMPONENTS QScintilla QUIET)

if(NOT Qt6QScintilla_FOUND)
    find_library(QSCINTILLA_LIBRARY qscintilla2_qt6 QUIET
        PATHS ${CMAKE_PREFIX_PATH}/lib
    )
    find_path(QSCINTILLA_INCLUDE_DIR Qsci/qsciscintilla.h
        PATHS /usr/include /usr/local/include /usr/include/qt6
              ${CMAKE_PREFIX_PATH}/include/qt6
              ${CMAKE_PREFIX_PATH}/include
    )
endif()

if(Qt6QScintilla_FOUND)
    set(QSCINTILLA_TARGET Qt6::QScintilla)
    message(STATUS "Found QScintilla via Qt6 config")
elseif(QSCINTILLA_LIBRARY AND QSCINTILLA_INCLUDE_DIR)
    set(QSCINTILLA_TARGET Qt6::QScintilla)
    add_library(${QSCINTILLA_TARGET} UNKNOWN IMPORTED)
    set_target_properties(${QSCINTILLA_TARGET} PROPERTIES
        IMPORTED_LOCATION ${QSCINTILLA_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${QSCINTILLA_INCLUDE_DIR}
    )
    message(STATUS "Found QScintilla via find_library")
elseif(BUILD_QSCINTILLA)
    message(STATUS "QScintilla not found; building from source")
    include(ExternalProject)

    find_program(QMAKE6 qmake6 REQUIRED
        DOC "qmake6 is required to build QScintilla from source"
    )

    ExternalProject_Add(ep_qscintilla
        URL https://www.riverbankcomputing.com/static/Downloads/QScintilla/2.14.1/QScintilla_src-2.14.1.tar.gz
        URL_HASH SHA256=dfe13c6acc9d85dfcba76ccc8061e71a223957a6c02f3c343b30a9d43a4cdd4d
        PREFIX ${DEPS_PREFIX}/qscintilla
        INSTALL_DIR ${DEPS_PREFIX}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND}
            -DQSCINTILLA_SRC_DIR=${DEPS_PREFIX}/qscintilla/src/ep_qscintilla
            -DQSCINTILLA_INSTALL_DIR=${DEPS_PREFIX}
            -P ${CMAKE_SOURCE_DIR}/cmake/build_qscintilla.cmake
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS ${DEPS_PREFIX}/lib/libqscintilla2_qt6.a
    )

    set(QSCINTILLA_TARGET Qt6::QScintilla)
    add_library(${QSCINTILLA_TARGET} UNKNOWN IMPORTED)
    file(MAKE_DIRECTORY ${DEPS_PREFIX}/include)
    set_target_properties(${QSCINTILLA_TARGET} PROPERTIES
        IMPORTED_LOCATION ${DEPS_PREFIX}/lib/libqscintilla2_qt6.a
        INTERFACE_INCLUDE_DIRECTORIES ${DEPS_PREFIX}/include
    )
    add_dependencies(${QSCINTILLA_TARGET} ep_qscintilla)
else()
    message(FATAL_ERROR
        "QScintilla not found. Install qscintilla2-qt6-dev or set -DBUILD_QSCINTILLA=ON"
    )
endif()

# ── QScintilla helper ─────────────────────────────────────────
function(target_link_qscintilla TARGET)
    target_link_libraries(${TARGET} PRIVATE ${QSCINTILLA_TARGET})
endfunction()

# ── Version header from CMake ────────────────────────────────
configure_file(
    src/devpad_version.h.in
    ${CMAKE_BINARY_DIR}/generated/devpad_version.h
)

# ── macOS Info.plist from CMake ──────────────────────────────
configure_file(
    resources/Info.plist.in
    ${CMAKE_BINARY_DIR}/generated/Info.plist
    @ONLY
)

# ── Core static library (headless, no Widgets) ──────────────
add_library(Devpad_core STATIC
    src/logger.cpp
    src/encodingutils.cpp
    src/gitignore.cpp
    src/core/fileservice.cpp
    src/tui/tuibuffer.cpp
    src/tui/tuitabmodel.cpp
    src/tui/tuisearchengine.cpp
    src/tui/tuifiletree.cpp
    src/tui/tuifindinfiles.cpp
    src/tui/tuihighlighter.cpp
    src/tui/tuiviewstate.cpp
    src/tui/tuirenderer.cpp
)
target_compile_options(Devpad_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(Devpad_core PUBLIC
    Qt6::Core
    Qt6::Network
)
# Position-independent code: Devpad_core is linked into plugin modules.
set_target_properties(Devpad_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(Devpad_core PUBLIC
    src
    src/managers
    ${CMAKE_BINARY_DIR}/generated
)

# ── Editor library (QScintilla + Qt Gui/Widgets) ────────────────────────
# Holds the editing core: QScintilla editor, themes, languages, snippets,
# and the lightweight editor widgets. SettingsManager lives here because its
# implementation includes codeeditor.h / QScintilla.
add_library(Devpad_editor STATIC
    src/codeeditor.cpp
    src/keywords.cpp
    src/languageinfo.cpp
    src/palettechangefilter.h
    src/snippet.cpp
    src/snippetengine.cpp
    src/theme.cpp
    src/managers/settingsmanager.cpp
    src/widgets/brackethelper.cpp
    src/widgets/inlinefindbar.cpp
    src/widgets/minimap.cpp
    src/widgets/editorcontainer.cpp
    src/widgets/themepreviewwidget.cpp
)
target_compile_options(Devpad_editor PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(Devpad_editor PUBLIC
    Devpad_core
    Qt6::Gui
    Qt6::Widgets
    ${QSCINTILLA_TARGET}
)
# Position-independent code: Devpad_editor is linked into plugin modules.
set_target_properties(Devpad_editor PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(TARGET Qt6::DBus)
    target_link_libraries(Devpad_editor PUBLIC Qt6::DBus)
    target_compile_definitions(Devpad_editor PUBLIC HAVE_QT_DBUS)
endif()
target_include_directories(Devpad_editor PUBLIC
    src
    src/managers
    ${CMAKE_BINARY_DIR}/generated
)

# ── LSP library ──────────────────────────────────────────────────────────
# JSON-RPC transport, language client, server manager and editor integration.
add_library(Devpad_lsp STATIC
    src/lsp/lspjsonrpc.cpp
    src/lsp/lsptypes.cpp
    src/lsp/lspclient.cpp
    src/lsp/lspservermanager.cpp
    src/lsp/lspeditorintegration.cpp
    src/lsp/lspindicators.h
    src/lsp/lspworkspaceedit.cpp
    src/lsp/lspworkspaceedit.h
)
target_compile_options(Devpad_lsp PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(Devpad_lsp PUBLIC
    Devpad_editor
    Qt6::Core
)
target_link_libraries(Devpad_lsp PRIVATE
    Qt6::Gui
    Qt6::Widgets
)
# Position-independent code: Devpad_lsp is linked into plugin modules.
set_target_properties(Devpad_lsp PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(Devpad_lsp PUBLIC
    src
    src/managers
    ${CMAKE_BINARY_DIR}/generated
)

# TERMINAL_HAS_GHOSTTY / TERMINAL_HAS_KODOTERM are only set in the branches
# above (libghostty / legacy); default them everywhere else so the
# $<BOOL:...> generator expressions below evaluate.
if(NOT DEFINED TERMINAL_HAS_GHOSTTY)
    set(TERMINAL_HAS_GHOSTTY FALSE)
endif()
if(NOT DEFINED TERMINAL_HAS_KODOTERM)
    set(TERMINAL_HAS_KODOTERM FALSE)
endif()

# ── Panels library ───────────────────────────────────────────────────────
# Dockable panels: project tree, split view, tab bar, error list, references,
# debugger panel, plus chrome widgets (title bar, window frame bridge).
add_library(Devpad_panels STATIC
    src/panels/projectpanel.cpp
    src/panels/splitview.cpp
    src/panels/draggabletabbar.cpp
    src/panels/dropzoneoverlay.cpp
    src/panels/errorlistpanel.cpp
    src/panels/referencespanel.cpp
    src/widgets/titlebar.cpp
    src/widgets/windowsframebridge.cpp
)
target_compile_options(Devpad_panels PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(Devpad_panels PUBLIC
    Devpad_editor
    Devpad_lsp
    Qt6::Widgets
)
target_include_directories(Devpad_panels PUBLIC
    src
    src/managers
    src/panels
    ${CMAKE_BINARY_DIR}/generated
)

# ── Terminal sources (built only into the terminal plugin) ─────────────────
# The terminal panel and backends live exclusively in the terminalplugin
# module so the heavyweight terminal widget (qtermwidget/KodoTerm) can be
# loaded at runtime and optionally left out of the main executable.
if(TERMINAL_HAS_GHOSTTY)
    set(DEVPAD_TERMINAL_SOURCES
        src/plugins/terminalplugin/terminalpanel.cpp
        src/plugins/terminalplugin/terminalbackend.cpp
        src/plugins/terminalplugin/terminalbackend_ghostty.cpp)
elseif(TERMINAL_HAS_KODOTERM)
    set(DEVPAD_TERMINAL_SOURCES
        src/plugins/terminalplugin/terminalpanel.cpp
        src/plugins/terminalplugin/terminalbackend.cpp
        src/plugins/terminalplugin/terminalbackend_kodoterm.cpp)
else()
    set(DEVPAD_TERMINAL_SOURCES
        src/plugins/terminalplugin/terminalpanel.cpp
        src/plugins/terminalplugin/terminalbackend.cpp
        src/plugins/terminalplugin/terminalbackend_qtermwidget.cpp)
endif()

# ── Terminal plugin ──────────────────────────────────────────────────────
# First-party plugin: creates and owns the TerminalPanel. Built as a MODULE
# library so it can be loaded at runtime via QLibrary from the user plugins
# directory.
if(BUILD_PLUGINS)
    add_library(terminalplugin MODULE
        src/plugins/terminalplugin/terminalplugin.cpp
        ${DEVPAD_TERMINAL_SOURCES}
    )
    target_compile_options(terminalplugin PRIVATE -Wall -Wextra -Wpedantic)
    if(DEVPAD_GHOSTTY_TERMINAL)
        target_compile_definitions(terminalplugin PRIVATE DEVPAD_GHOSTTY_TERMINAL)
    endif()
    target_include_directories(terminalplugin PRIVATE
        src
        src/managers
        src/panels
    )
    target_link_libraries(terminalplugin PRIVATE
        # Plugins resolve Devpad's own classes/symbols from the main executable
        # (which exports them via ENABLE_EXPORTS), so they share the app's
        # singletons (SettingsManager, Logger, ...) instead of getting a
        # private static copy.
        ${TERMINAL_TARGET}
        Qt6::Core
        Qt6::Widgets
    )
    target_link_qscintilla(terminalplugin)
    set_target_properties(terminalplugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        PREFIX "lib"
        SUFFIX ".so"
        OUTPUT_NAME terminalplugin
    )
    # Copy the plugin manifest next to the shared library.
    configure_file(
        src/plugins/terminalplugin/terminalplugin.plugin.json
        ${CMAKE_BINARY_DIR}/plugins/terminalplugin.plugin.json
        COPYONLY
    )

    # ── DAP debugger plugin ──────────────────────────────────────────
    # First-party plugin: provides Debug Adapter Protocol debugging support.
    # Creates DapServerManager, DebugPanel, and per-editor DapEditorIntegration.
    if(BUILD_PLUGINS)
        add_library(dap_plugin MODULE
            src/plugins/dap_plugin/dapplugin.cpp
            src/plugins/dap_plugin/dapclient.cpp
            src/plugins/dap_plugin/dapservermanager.cpp
            src/plugins/dap_plugin/daptypes.cpp
            src/plugins/dap_plugin/dapeditorintegration.cpp
            src/plugins/dap_plugin/debugpanel.cpp
        )
        target_compile_options(dap_plugin PRIVATE -Wall -Wextra -Wpedantic)
        target_include_directories(dap_plugin PRIVATE
            src
            src/managers
            src/panels
            ${CMAKE_BINARY_DIR}/generated
        )
        target_link_libraries(dap_plugin PRIVATE
            # Plugins resolve Devpad's own classes/symbols from the main executable
            # (which exports them via ENABLE_EXPORTS), so they share the app's
            # singletons (SettingsManager, Logger, ...) instead of getting a
            # private static copy.
            Qt6::Core
            Qt6::Widgets
        )
        target_link_qscintilla(dap_plugin)
        set_target_properties(dap_plugin PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
            PREFIX "lib"
            SUFFIX ".so"
            OUTPUT_NAME dap_plugin
        )
        # Copy the plugin manifest next to the shared library.
        configure_file(
            src/plugins/dap_plugin/dap_plugin.plugin.json
            ${CMAKE_BINARY_DIR}/plugins/dap_plugin.plugin.json
            COPYONLY
        )
    endif()
endif()

# ── App / coordinator static library (slimmed Devpad_lib) ────────────────
# Holds MainWindow, EditorController, the remaining managers/dialogs/TUI and
# the plugin host. Keeps the historic Devpad_lib name so the Devpad executable
# and Devpad_tests need no link/include changes.
add_library(Devpad_lib STATIC
    src/mainwindow.cpp
    src/editorcontroller.cpp
    src/themeapplier.cpp
    src/managers/backupmanager.cpp
    src/managers/externaltoolmanager.cpp
    src/managers/filemanager.cpp
    src/managers/remotefileservice.cpp
    src/managers/tabmanager.cpp
    src/managers/searchmanager.cpp
    src/managers/snippetmanager.cpp
    src/managers/actionmanager.cpp
    src/managers/recentfileshelper.cpp
    src/managers/encodingmenuhelper.cpp
    src/managers/sessionmanager.cpp
    src/managers/encodingmanager.cpp
    src/managers/printmanager.cpp
    src/managers/filewatchermanager.cpp
    src/managers/updatechecker.cpp
    src/managers/externalchangecoordinator.cpp
    src/managers/remoteopencoordinator.cpp
    src/managers/updateservicecoordinator.cpp
    src/managers/remoteservicecoordinator.cpp
    src/managers/terminalpanelcoordinator.cpp
    src/dialogs/findinfilesdialog.cpp
    src/dialogs/findsymbolsdialog.cpp
    src/dialogs/dialogcoordinator.cpp
    src/dialogs/aboutdialog.cpp
    src/dialogs/customthemedialog.cpp
    src/dialogs/optionsdialog.cpp
    src/dialogs/externaltoolsdialog.cpp
    src/dialogs/updatedialog.cpp
    src/lsp/lspconnectionhelper.cpp
    src/tui/tuisession.cpp
    src/tui/tuicommand.cpp
    src/tui/tuiinput.cpp
    $<$<BOOL:${BUILD_PLUGINS}>:src/plugins/pluginhost.cpp>
    $<$<BOOL:${BUILD_PLUGINS}>:src/plugins/pluginmanager.cpp>
)
target_compile_options(Devpad_lib PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(Devpad_lib PUBLIC
    Devpad_core
    Devpad_editor
    Devpad_lsp
    Devpad_panels
    Qt6::PrintSupport
    Qt6::Multimedia
    Qt6::Svg
)
if(TARGET Qt6::DBus)
    target_link_libraries(Devpad_lib PUBLIC Qt6::DBus)
    target_compile_definitions(Devpad_lib PUBLIC HAVE_QT_DBUS)
endif()
if(BUILD_PLUGINS)
    target_compile_definitions(Devpad_lib PUBLIC BUILD_PLUGINS=1
        "DEVPAD_SYSTEM_PLUGINS_DIR=\"${CMAKE_INSTALL_PREFIX}/lib/devpad/plugins\"")
endif()
target_include_directories(Devpad_lib PUBLIC
    src
    src/managers
    src/dialogs
    src/panels
    ${CMAKE_BINARY_DIR}/generated
)
target_link_qscintilla(Devpad_lib)

# ── TUI mode ──────────────────────────────────────────────────
if(BUILD_TUI)
    find_package(Curses REQUIRED)
    message(STATUS "Building with TUI support (ncurses ${CURSES_VERSION})")
endif()

# ── Next-gen / QML mode ─────────────────────────────────────────
if(BUILD_NEXTGEN)
    find_package(Qt6 6.6 REQUIRED COMPONENTS Quick Qml QuickControls2)
    message(STATUS "Building with NEXTGEN support (QML + primoEditor)")
endif()

# ── Main executable ───────────────────────────────────────────
add_executable(Devpad WIN32)
# Export symbols so plugins can qobject_cast host objects (e.g. the shared
# SettingsManager instance) across the DSO boundary.
set_target_properties(Devpad PROPERTIES ENABLE_EXPORTS ON)

target_sources(Devpad PRIVATE
    src/main.cpp
    resources/resources.qrc
    $<$<PLATFORM_ID:Windows>:resources/appicon.rc>
    $<$<BOOL:${BUILD_TUI}>:src/tui/tuiapp.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/nextgenapp.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/primoeditor.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/primodocument.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/primohighlighter.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/primoshader.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/nextgenactions.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/nextgentabmodel.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/primoterminal.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/primofindinfiles.cpp>
    $<$<BOOL:${BUILD_NEXTGEN}>:src/nextgen/filetreemodel.cpp>
)

target_compile_options(Devpad PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(Devpad PRIVATE
    Devpad_lib
    Devpad_core
)
target_link_qscintilla(Devpad)

target_include_directories(Devpad PRIVATE
    src
)

if(BUILD_TUI)
    target_compile_definitions(Devpad PRIVATE BUILD_TUI=1)
    target_compile_definitions(Devpad_core PRIVATE BUILD_TUI=1)
    target_compile_definitions(Devpad_lib PRIVATE BUILD_TUI=1)
    target_include_directories(Devpad PRIVATE ${CURSES_INCLUDE_DIRS})
    target_include_directories(Devpad_core PRIVATE ${CURSES_INCLUDE_DIRS})
    target_include_directories(Devpad_lib PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(Devpad PRIVATE ${CURSES_LIBRARIES})
endif()

if(BUILD_NEXTGEN)
    target_compile_definitions(Devpad PRIVATE BUILD_NEXTGEN=1)
    target_compile_definitions(Devpad_core PRIVATE BUILD_NEXTGEN=1)
    target_compile_definitions(Devpad_lib PRIVATE BUILD_NEXTGEN=1)
    target_link_libraries(Devpad PRIVATE
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
    )
endif()

# ── Install rules ─────────────────────────────────────────────
install(TARGETS Devpad RUNTIME DESTINATION bin)

install(FILES resources/com.semagsoft.Devpad.desktop
    DESTINATION share/applications
)

install(FILES resources/com.semagsoft.Devpad.metainfo.xml
    DESTINATION share/metainfo
)

install(FILES resources/icons/devpad.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME com.semagsoft.Devpad.svg
)

install(FILES LICENSE
    DESTINATION share/licenses/Devpad
)

# ── Install first-party plugins ───────────────────────────────
# System plugins are installed to lib/devpad/plugins (read-only,
# shipped with the app). User-installed plugins live in
# ~/.local/share/devpad/plugins/ and are scanned in addition.
if(BUILD_PLUGINS)
    install(FILES
        ${CMAKE_BINARY_DIR}/plugins/libterminalplugin.so
        ${CMAKE_BINARY_DIR}/plugins/terminalplugin.plugin.json
        ${CMAKE_BINARY_DIR}/plugins/libdap_plugin.so
        ${CMAKE_BINARY_DIR}/plugins/dap_plugin.plugin.json
        DESTINATION lib/devpad/plugins
        OPTIONAL
    )
endif()

# ── Internationalization ──────────────────────────────────────
set(TS_FILES
    i18n/devpad_de.ts
    i18n/devpad_es.ts
    i18n/devpad_fr.ts
)
qt_add_translations(Devpad
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE qm_files
)
target_sources(Devpad PRIVATE ${qm_files})

# ── Tests ─────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)

# ── Translations ──────────────────────────────────────────────
find_program(QT_LUPDATE NAMES lupdate-qt6 lupdate HINTS ${Qt6_DIR}/../../../bin /usr/lib/qt6/bin)
if(QT_LUPDATE)
    add_custom_target(update-translations
        COMMAND ${QT_LUPDATE} -no-obsolete src/ -ts ${TS_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Updating translation .ts files from source"
    )
endif()

# ── Formatting ────────────────────────────────────────────────
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCES
        src/*.cpp src/*.h src/*.hpp
        tests/*.cpp tests/*.h tests/*.hpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting all source files with clang-format"
    )
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking formatting with clang-format"
    )
endif()

# ── Clang-tidy ────────────────────────────────────────────────
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-18 clang-tidy-17)
find_package(Python3 COMPONENTS Interpreter)
if(CLANG_TIDY AND Python3_Interpreter_FOUND)
    # Mirrors the CI "Clang-tidy" job: lint only files with a compile_commands.json
    # entry, so OFF-by-default options (BUILD_TUI, BUILD_NEXTGEN) are skipped.
    # Requires a configured build dir exporting the DB (default: build/).
    add_custom_target(check-tidy
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run-clang-tidy.py
                --build-dir $<IF:$<BOOL:${CMAKE_EXPORT_COMPILE_COMMANDS}>,${CMAKE_BINARY_DIR},${CMAKE_SOURCE_DIR}/build>
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking sources with clang-tidy"
        VERBATIM
    )
endif()

if(BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    include(FetchContent)

    # Always fetch googletest to ensure compiler compatibility
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    include(GoogleTest)

    set(TEST_SOURCES
        tests/main.cpp
        tests/test_integration.cpp
        tests/test_encodingutils.cpp
        tests/test_encodingmanager.cpp
        tests/test_keywords.cpp
        tests/test_appstrings.cpp
        tests/test_actionmanager.cpp
        tests/test_logger.cpp
        tests/test_backupmanager.cpp
        tests/test_filemanager.cpp
        tests/test_settingsmanager.cpp
        tests/test_remotefileservice.cpp
        tests/test_codeeditor.cpp
        tests/test_tabmanager.cpp
        tests/test_searchmanager.cpp
        tests/test_theme.cpp
        tests/test_themeapplier.cpp
        tests/test_editorcontroller.cpp
        tests/test_mainwindow.cpp
        tests/test_languageinfo.cpp
        tests/test_sessionmanager.cpp
        tests/test_snippet.cpp
        tests/test_snippetmanager.cpp
        tests/test_snippetengine.cpp
        tests/test_editorcontainer.cpp
        tests/test_encodingmenuhelper.cpp
        tests/test_externaltoolmanager.cpp
        tests/test_findinfilesworker.cpp
        tests/test_filewatchermanager.cpp
        tests/test_gitignore.cpp
        tests/test_inlinefindbar.cpp
        tests/test_titlebar.cpp
        tests/test_printmanager.cpp
        tests/test_recentfileshelper.cpp
        tests/test_lsptypes.cpp
        tests/test_lspjsonrpc.cpp
        tests/test_lspservermanager.cpp
        tests/test_workspaceedit.cpp
        tests/test_splitview.cpp
        tests/test_updatechecker.cpp
        tests/test_projectpanel.cpp
        tests/test_lspclient.cpp
        tests/test_fileservice.cpp
        tests/test_tuibuffer.cpp
        tests/test_tuitabmodel.cpp
        tests/test_tuisearchengine.cpp
        tests/test_tuifiletree.cpp
        tests/test_tuihighlighter.cpp
        tests/test_tuifindinfiles.cpp
        tests/test_tuicommand.cpp
    )

    add_executable(Devpad_tests ${TEST_SOURCES} resources/resources.qrc)
    # Tests load plugins too; export symbols for cross-DSO qobject_cast.
    set_target_properties(Devpad_tests PROPERTIES ENABLE_EXPORTS ON)

    target_compile_options(Devpad_tests PRIVATE -Wall -Wextra -Wpedantic)

    target_link_libraries(Devpad_tests PRIVATE
        GTest::gtest
        GTest::gmock
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Test
        Devpad_lib
    )
    target_link_qscintilla(Devpad_tests)

    target_include_directories(Devpad_tests PRIVATE
        src
        src/managers
        src/dialogs
        src/panels
    )

    gtest_discover_tests(Devpad_tests
        DISCOVERY_MODE PRE_TEST
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
