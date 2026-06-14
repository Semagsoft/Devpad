cmake_minimum_required(VERSION 3.23)
project(Devpad VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets PrintSupport Multimedia LinguistTools
)
find_package(qtermwidget6 REQUIRED)

find_package(Qt6 COMPONENTS QScintilla QUIET)
if(Qt6QScintilla_FOUND)
    set(QSCINTILLA_TARGET Qt6::QScintilla)
    message(STATUS "Found QScintilla via Qt6 config")
else()
    find_library(QSCINTILLA_LIBRARY qscintilla2_qt6 REQUIRED)
    find_path(QSCINTILLA_INCLUDE_DIR Qsci/qsciscintilla.h
        PATHS /usr/include /usr/local/include /usr/include/qt6
    )
    if(NOT QSCINTILLA_INCLUDE_DIR)
        message(FATAL_ERROR "QScintilla headers not found")
    endif()
    message(STATUS "Found QScintilla via find_library")
endif()

# ── QScintilla helper ─────────────────────────────────────────
function(target_link_qscintilla TARGET)
    if(Qt6QScintilla_FOUND)
        target_link_libraries(${TARGET} PRIVATE ${QSCINTILLA_TARGET})
    else()
        target_link_libraries(${TARGET} PRIVATE ${QSCINTILLA_LIBRARY})
        target_include_directories(${TARGET} PRIVATE ${QSCINTILLA_INCLUDE_DIR})
    endif()
endfunction()

# ── Shared static library (avoids double compilation) ─────────
add_library(Devpad_lib STATIC
    src/codeeditor.cpp
    src/keywords.cpp
    src/languageinfo.cpp
    src/theme.cpp
    src/logger.cpp
    src/encodingutils.cpp
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
    src/managers/sessionmanager.cpp
    src/managers/encodingmanager.cpp
    src/managers/printmanager.cpp
    src/managers/filewatchermanager.cpp
    src/snippet.cpp
    src/dialogs/finddialog.cpp
    src/dialogs/replacedialog.cpp
    src/dialogs/findinfilesdialog.cpp
    src/dialogs/aboutdialog.cpp
    src/dialogs/optionsdialog.cpp
    src/dialogs/externaltoolsdialog.cpp
    src/panels/projectpanel.cpp
    src/panels/terminalpanel.cpp
    src/panels/splitview.cpp
    src/widgets/themepreviewwidget.cpp
    src/codeeditor.h
    src/snippet.h
    src/theme.h
    src/languageinfo.h
    src/logger.h
    src/encodingutils.h
    src/keywords.h
    src/defaultsyntax.h
    src/appstrings.h
    src/managers/backupmanager.h
    src/managers/externaltoolmanager.h
    src/managers/filemanager.h
    src/managers/settingsmanager.h
    src/managers/remotefileservice.h
    src/managers/tabmanager.h
    src/managers/searchmanager.h
    src/mainwindow.h
    src/editorcontroller.h
    src/themeapplier.h
    src/managers/actionmanager.h
    src/managers/sessionmanager.h
    src/managers/encodingmanager.h
    src/managers/printmanager.h
    src/managers/filewatchermanager.h
    src/managers/snippetmanager.h
    src/dialogs/finddialog.h
    src/dialogs/dialogsettings.h
    src/dialogs/replacedialog.h
    src/dialogs/findinfilesdialog.h
    src/dialogs/aboutdialog.h
    src/dialogs/optionsdialog.h
    src/dialogs/externaltoolsdialog.h
    src/panels/projectpanel.h
    src/panels/terminalpanel.h
    src/panels/splitview.h
    src/widgets/themepreviewwidget.h
)
target_compile_options(Devpad_lib PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(Devpad_lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Multimedia
    Qt6::Network
    qtermwidget6
)
target_include_directories(Devpad_lib PUBLIC
    src
    src/managers
    src/dialogs
    src/panels
)
target_link_qscintilla(Devpad_lib)

# ── Main executable ───────────────────────────────────────────
add_executable(Devpad)

target_sources(Devpad PRIVATE
    src/main.cpp
    resources/resources.qrc
)

target_compile_options(Devpad PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(Devpad PRIVATE
    Devpad_lib
)
target_link_qscintilla(Devpad)

target_include_directories(Devpad PRIVATE
    src
)

install(TARGETS Devpad RUNTIME DESTINATION bin)

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

    set(TEST_SOURCES
        tests/main.cpp
        tests/test_encodingutils.cpp
        tests/test_keywords.cpp
        tests/test_appstrings.cpp
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
    )

    add_executable(Devpad_tests ${TEST_SOURCES})

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

    add_test(NAME Devpad_tests COMMAND Devpad_tests)
endif()
