# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools")
  file(MAKE_DIRECTORY "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools")
endif()
file(MAKE_DIRECTORY
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools-build"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/tmp"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools-stamp"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src"
  "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/primo/Work/repos/Devpad/build-tui-release/_deps/lxqt-build-tools/src/ep_lxqt_build_tools-stamp${cfgdir}") # cfgdir has leading slash
endif()
