# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla")
  file(MAKE_DIRECTORY "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla")
endif()
file(MAKE_DIRECTORY
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla-build"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/tmp"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla-stamp"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/primo/Work/repos/Devpad/build-tui-release/_deps/qscintilla/src/ep_qscintilla-stamp${cfgdir}") # cfgdir has leading slash
endif()
