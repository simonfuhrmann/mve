cmake_minimum_required (VERSION 3.13)

# use project VERSION
cmake_policy(SET CMP0048 NEW)

project(mve LANGUAGES CXX VERSION 1.0.0)

option(MVE_BUILD_UMVE "Build UMVE module" OFF)
include(GNUInstallDirs)

add_subdirectory(libs)
add_subdirectory(apps)