cmake_minimum_required (VERSION 3.1)

option(MVE_BUILD_UMVE "Build UMVE module" OFF)
include(GNUInstallDirs)

add_subdirectory(libs)
add_subdirectory(apps)