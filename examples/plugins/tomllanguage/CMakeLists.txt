# Example Devpad plugin: custom language "TOML"
#
# This plugin demonstrates the participation hooks:
#   - registerLanguage(): contribute a new language/lexer
#   - registerLanguageKeywords(): contribute keywords
#   - registerExtension(): map file extensions to the language
#   - registerEncoding(): contribute a custom text encoding
#
# Build (from this directory):
#   cmake -B build -DCMAKE_PREFIX_PATH=<path to Qt6> \
#         -DDEVPAD_SRC_DIR=<path to Devpad/src>
#   cmake --build build -j
#
# Install: copy build/libtomllanguage.so + tomllanguage.plugin.json to
#   ~/.local/share/devpad/plugins/  (Linux/macOS)
#   or %APPDATA%\devpad\plugins\     (Windows)
#   Then restart Devpad and open a .toml file to see syntax highlighting.

cmake_minimum_required(VERSION 3.23)
project(tomllanguage_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEVPAD_SRC_DIR)
    message(FATAL_ERROR "DEVPAD_SRC_DIR must point at the Devpad src/ directory")
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)

add_library(tomllanguage MODULE tomllanguage.cpp)
target_include_directories(tomllanguage PRIVATE ${DEVPAD_SRC_DIR})
target_link_libraries(tomllanguage PRIVATE Qt6::Core Qt6::Widgets)

# Plugin libraries are named lib<name>.so on Unix / <name>.dll on Windows.
set_target_properties(tomllanguage PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    OUTPUT_NAME tomllanguage
)
