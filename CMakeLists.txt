cmake_minimum_required (VERSION 3.1)

project(MVE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED yes)

# Warning flags
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wundef -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -pedantic")
endif()

if(WIN32 AND NOT UNIX)
	if(MSVC)
		set(MVE_BUILD_WITH_SSE2 yes CACHE BOOL "Build MVE with SSE2 support")
		set(MVE_BUILD_WITH_SSE3 yes CACHE BOOL "Build MVE with SSE3 support")
		if(MVE_BUILD_WITH_SSE2)
			add_definitions(-D__SSE2__)
		endif()
		if(MVE_BUILD_WITH_SSE3)
			add_definitions(-D__SSE3__)
		endif()
	endif()

	set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty" "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qt5/src/qt5/qtbase/lib/cmake/Qt5")
	set(JPEG_NAMES "libjpeg")
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

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# Install 3rd party libraries
if(WIN32)
	set(LIBPNG_DLL_DEBUG "3rdparty/bin/libpng16d.dll")
	set(LIBPNG_DLL_RELEASE "3rdparty/bin/libpng16.dll")
	set(ZLIB_DLL_RELEASE "3rdparty/bin/zlib.dll")
	set(LIBTIFF_DLL_RELEASE "3rdparty/bin/libtiff.dll")
	install(FILES ${LIBPNG_DLL_RELEASE} ${ZLIB_DLL_RELEASE}
		${LIBTIFF_DLL_RELEASE} DESTINATION . CONFIGURATIONS Release RelWithDebInfo)
	install(FILES ${LIBPNG_DLL_DEBUG} ${ZLIB_DLL_RELEASE}
		${LIBTIFF_DLL_RELEASE} DESTINATION . CONFIGURATIONS Debug)
endif()

add_subdirectory(libs)
add_subdirectory(apps)
#add_subdirectory(tests)
