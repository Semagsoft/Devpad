cmake_minimum_required(VERSION 3.23)
project(example_plugins LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEVPAD_SRC_DIR)
    message(FATAL_ERROR "DEVPAD_SRC_DIR must point at the Devpad src/ directory")
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)

# ── Example plugin: custom snippets + theme + file watcher ─────────
add_library(example_plugin MODULE example_plugin.cpp)
target_include_directories(example_plugin PRIVATE ${DEVPAD_SRC_DIR})
target_link_libraries(example_plugin PRIVATE Qt6::Core Qt6::Widgets)
set_target_properties(example_plugin PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    OUTPUT_NAME example_plugin
)
configure_file(
    example_plugin.plugin.json
    ${CMAKE_CURRENT_BINARY_DIR}/example_plugin.plugin.json
    COPYONLY
)
