cmake_minimum_required (VERSION 2.6)

project(MVE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32 AND NOT UNIX)
    set(CPACK_GENERATOR "ZIP")
else(WIN32 AND NOT UNIX)
    set(CPACK_GENERATOR "TGZ")
endif(WIN32 AND NOT UNIX)
include(CPack)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(OpenMP REQUIRED)

add_definitions(${PNG_DEFINITIONS})
if(WIN32)
    add_definitions(-DGLEW_STATIC)
	add_definitions(-DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
endif()
include_directories(${JPEG_INCLUDE_DIR})
include_directories(${PNG_INCLUDE_DIRS})
include_directories(${TIFF_INCLUDE_DIR})
include_directories(libs)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

add_subdirectory(libs)
add_subdirectory(apps)
#add_subdirectory(tests)
