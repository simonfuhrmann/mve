cmake_minimum_required (VERSION 3.13)

# use project VERSION
cmake_policy(SET CMP0048 NEW)

project(mve LANGUAGES CXX VERSION 1.0.0)

option(MVE_BUILD_UMVE "Build UMVE module" OFF)
option(MVE_BUILD_APPS "Build the applications" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(GNUInstallDirs)

add_subdirectory(libs)

if(MVE_BUILD_APPS)
    add_subdirectory(apps)
endif()