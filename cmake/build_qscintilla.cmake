# cmake/build_qscintilla.cmake
# Standalone CMake script to download, build, and install QScintilla from source.
# Usage: cmake -DQSCINTILLA_SRC_DIR=<src> -DQSCINTILLA_INSTALL_DIR=<prefix> -P build_qscintilla.cmake
#
# Requires: qmake6 on PATH
#
# Variables:
#   QSCINTILLA_SRC_DIR     - Directory containing the extracted QScintilla source
#   QSCINTILLA_INSTALL_DIR - Install prefix (lib/ and include/ go here)

cmake_minimum_required(VERSION 3.23)

# ── Required inputs ──────────────────────────────────────────
set(QSCINTILLA_SRC_DIR "" CACHE PATH "QScintilla extracted source directory")
set(QSCINTILLA_INSTALL_DIR "" CACHE PATH "Install prefix (lib/ and include/ go here)")

if(NOT QSCINTILLA_SRC_DIR OR NOT QSCINTILLA_INSTALL_DIR)
    message(FATAL_ERROR "QSCINTILLA_SRC_DIR and QSCINTILLA_INSTALL_DIR must be set")
endif()

# ── Find qmake ───────────────────────────────────────────────
find_program(QMAKE6 NAMES qmake6 qmake REQUIRED)
message(STATUS "Using qmake: ${QMAKE6}")

# ── Locate the actual source root ────────────────────────────
# The tarball may or may not have a top-level directory.
set(QSCI_ROOT "")
if(EXISTS ${QSCINTILLA_SRC_DIR}/src/qscintilla.pro)
    set(QSCI_ROOT ${QSCINTILLA_SRC_DIR})
else()
    file(GLOB children RELATIVE ${QSCINTILLA_SRC_DIR} ${QSCINTILLA_SRC_DIR}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${QSCINTILLA_SRC_DIR}/${child}
           AND EXISTS ${QSCINTILLA_SRC_DIR}/${child}/src/qscintilla.pro)
            set(QSCI_ROOT ${QSCINTILLA_SRC_DIR}/${child})
            break()
        endif()
    endforeach()
endif()

if(NOT QSCI_ROOT)
    message(FATAL_ERROR
        "Cannot find qscintilla.pro in ${QSCINTILLA_SRC_DIR} "
        "(looked for src/qscintilla.pro at root and one level deep)"
    )
endif()

message(STATUS "QScintilla source root: ${QSCI_ROOT}")

# ── Detect parallelism ───────────────────────────────────────
include(ProcessorCount)
ProcessorCount(NPROC)
if(NPROC EQUAL 0)
    set(NPROC 2)
endif()
message(STATUS "Building with ${NPROC} parallel jobs")

# ── Build directory ──────────────────────────────────────────
set(QSCI_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/qscintilla-build)
file(MAKE_DIRECTORY ${QSCI_BUILD_DIR})

# ── Configure (qmake) ────────────────────────────────────────
message(STATUS "Configuring QScintilla with qmake...")
execute_process(
    COMMAND ${QMAKE6} ${QSCI_ROOT}/src/qscintilla.pro
        -after
        "target.path=${QSCINTILLA_INSTALL_DIR}/lib"
        "header.path=${QSCINTILLA_INSTALL_DIR}/include"
        "trans.path=${QSCINTILLA_INSTALL_DIR}/translations/QScintilla"
        "qsci.path=${QSCINTILLA_INSTALL_DIR}/qsci"
        "features.path=${QSCINTILLA_INSTALL_DIR}/mkspecs/features"
    WORKING_DIRECTORY ${QSCI_BUILD_DIR}
    RESULT_VARIABLE result
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(result)
    message(FATAL_ERROR "qmake configuration failed (${result}):\n${error}")
endif()
message(STATUS "qmake configuration OK")

# ── Build ─────────────────────────────────────────────────────
message(STATUS "Building QScintilla...")
execute_process(
    COMMAND make -j${NPROC}
    WORKING_DIRECTORY ${QSCI_BUILD_DIR}
    RESULT_VARIABLE result
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(result)
    message(FATAL_ERROR "QScintilla build failed (${result}):\n${error}")
endif()
message(STATUS "QScintilla build OK")

# ── Install ───────────────────────────────────────────────────
message(STATUS "Installing QScintilla to ${QSCINTILLA_INSTALL_DIR}...")
execute_process(
    COMMAND make install
    WORKING_DIRECTORY ${QSCI_BUILD_DIR}
    RESULT_VARIABLE result
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(result)
    message(FATAL_ERROR "QScintilla install failed (${result}):\n${error}")
endif()
message(STATUS "QScintilla build complete")
