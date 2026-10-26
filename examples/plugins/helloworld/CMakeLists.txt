# Example Devpad plugin: Hello World
#
# Build:
#   cmake -B build -DCMAKE_PREFIX_PATH=<Qt6 dir> -DDEVPAD_SRC_DIR=<Devpad/src>
#   cmake --build build -j
#
# Install: copy build/libhelloworld.so + helloworld.plugin.json to
#   ~/.local/share/devpad/plugins/ (Linux/macOS) or %APPDATA%\Devpad\plugins\ (Windows)

cmake_minimum_required(VERSION 3.23)
project(helloworld_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEVPAD_SRC_DIR)
    message(FATAL_ERROR "DEVPAD_SRC_DIR must point at the Devpad src/ directory")
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)

add_library(helloworld MODULE helloworld.cpp)
target_include_directories(helloworld PRIVATE ${DEVPAD_SRC_DIR})
target_link_libraries(helloworld PRIVATE Qt6::Core Qt6::Widgets)

# Plugin libraries are named lib<name>.so on Unix / <name>.dll on Windows.
set_target_properties(helloworld PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    OUTPUT_NAME helloworld
)
