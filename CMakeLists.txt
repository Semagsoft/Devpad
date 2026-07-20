cmake_minimum_required(VERSION 3.23)
project(Devpad VERSION 1.0 LANGUAGES CXX)

set(DEPS_PREFIX ${CMAKE_BINARY_DIR}/_deps)
option(BUILD_QSCINTILLA "Build QScintilla from source if not found system-wide" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core Gui Widgets PrintSupport Multimedia Network Svg LinguistTools
)
# ── Terminal widget ──────────────────────────────────────────
if(WIN32)
    include(FetchContent)
    message(STATUS "Fetching KodoTerm terminal widget (Windows)...")
    FetchContent_Declare(KodoTerm
        GIT_REPOSITORY https://github.com/diegoiast/KodoTerm.git
        GIT_TAG main
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
            PATCH_COMMAND sh -c "sed -i 's/set(LXQT_MIN_LINGUIST_VERSION \"6.6\")/set(LXQT_MIN_LINGUIST_VERSION \"6.2\")/' CMakeLists.txt"
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX}
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
        )

        ExternalProject_Add(ep_qtermwidget
            GIT_REPOSITORY https://github.com/lxqt/qtermwidget.git
            GIT_TAG 2.1.0
            PREFIX ${DEPS_PREFIX}/qtermwidget
            PATCH_COMMAND sh -c "sed -i.bak 's/ SHARED / STATIC /' CMakeLists.txt && sed -i 's/set(QT_MINIMUM_VERSION \"6.6.0\")/set(QT_MINIMUM_VERSION \"6.2.0\")/' CMakeLists.txt && rm -f CMakeLists.txt.bak"
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX}
                -DCMAKE_PREFIX_PATH=${DEPS_PREFIX}
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

# ── QScintilla (system or build from source) ──────────────────
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

# ── Shared static library (avoids double compilation) ─────────
add_library(Devpad_lib STATIC
    src/codeeditor.cpp
    src/keywords.cpp
    src/languageinfo.cpp
    src/theme.cpp
    src/logger.cpp
    src/encodingutils.cpp
    src/gitignore.cpp
    src/mainwindow.cpp
    src/editorcontroller.cpp
    src/themeapplier.cpp
    src/managers/backupmanager.cpp
    src/managers/externaltoolmanager.cpp
    src/managers/filemanager.cpp
    src/managers/settingsmanager.cpp
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
    src/snippet.cpp
    src/snippetengine.cpp
    src/dialogs/findinfilesdialog.cpp
    src/dialogs/findsymbolsdialog.cpp
    src/dialogs/aboutdialog.cpp
    src/dialogs/optionsdialog.cpp
    src/dialogs/externaltoolsdialog.cpp
    src/panels/projectpanel.cpp
    src/panels/terminalpanel.cpp
    src/panels/terminalbackend.cpp
    src/panels/splitview.cpp
    $<$<NOT:$<BOOL:${TERMINAL_HAS_KODOTERM}>>:src/panels/terminalbackend_qtermwidget.cpp>
    $<$<BOOL:${TERMINAL_HAS_KODOTERM}>:src/panels/terminalbackend_kodoterm.cpp>
    src/panels/draggabletabbar.cpp
    src/panels/errorlistpanel.cpp
    src/widgets/themepreviewwidget.cpp
    src/widgets/inlinefindbar.cpp
    src/widgets/editorcontainer.cpp
    src/lsp/lspjsonrpc.cpp
    src/lsp/lspclient.cpp
    src/lsp/lspservermanager.cpp
    src/lsp/lsptypes.cpp
    src/lsp/lspeditorintegration.cpp
)
target_compile_options(Devpad_lib PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(Devpad_lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Multimedia
    Qt6::Network
    Qt6::Svg
    ${TERMINAL_TARGET}
)
target_include_directories(Devpad_lib PUBLIC
    src
    src/managers
    src/dialogs
    src/panels
    ${CMAKE_BINARY_DIR}/generated
)
target_link_qscintilla(Devpad_lib)

# ── Main executable ───────────────────────────────────────────
add_executable(Devpad WIN32)

target_sources(Devpad PRIVATE
    src/main.cpp
    resources/resources.qrc
    $<$<PLATFORM_ID:Windows>:resources/appicon.rc>
)

target_compile_options(Devpad PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(Devpad PRIVATE
    Devpad_lib
)
target_link_qscintilla(Devpad)

target_include_directories(Devpad PRIVATE
    src
)

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

if(BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    include(FetchContent)

    # Prefer system-installed GTest over fetching from source
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

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
        tests/test_externaltoolmanager.cpp
        tests/test_printmanager.cpp
        tests/test_filewatchermanager.cpp
        tests/test_lsptypes.cpp
    )

    add_executable(Devpad_tests ${TEST_SOURCES} resources/resources.qrc)

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

    gtest_discover_tests(Devpad_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
