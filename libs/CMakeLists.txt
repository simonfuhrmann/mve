# Warning flags
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wundef -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -pedantic")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MVE_BUILD_WITH_SSE2 "Build MVE with SSE2 support" ON)
option(MVE_BUILD_WITH_SSE3 "Build MVE with SSE3 support" ON)
option(MVE_NO_PNG_SUPPORT "Build MVE without PNG support" OFF)
option(MVE_NO_JPEG_SUPPORT "Build MVE without JPEG support" OFF)
option(MVE_NO_TIFF_SUPPORT "Build MVE without TIFF support" OFF)

if(NOT MVE_NO_JPEG_SUPPORT)
    find_package(JPEG REQUIRED)
endif()
if(NOT MVE_NO_PNG_SUPPORT)
    find_package(PNG REQUIRED)
endif()
if(NOT MVE_NO_TIFF_SUPPORT)
    find_package(TIFF REQUIRED)
endif()

find_package(OpenMP QUIET)

if(NOT WIN32)
    find_package(Threads REQUIRED)
endif()

####

# Layout. This works for all platforms:
#   * <prefix>/lib/cmake/<PROJECT-NAME>
#   * <prefix>/lib/
#   * <prefix>/include/
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(include_install_dir "${CMAKE_INSTALL_INCLUDEDIR}")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")


add_subdirectory(util)
add_subdirectory(math)
add_subdirectory(mve)
add_subdirectory(fssr)
add_subdirectory(sfm)
add_subdirectory(dmrecon)
add_subdirectory(ogl)

# Include module with function 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Note: PROJECT_VERSION is used as a VERSION
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * targets_export_name
#   * PROJECT_NAME
configure_package_config_file(
    "cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

export(EXPORT ${targets_export_name} FILE "${generated_dir}/${targets_export_name}.cmake")

# Config
#   * <prefix>/lib/cmake/mve/mveConfig.cmake
#   * <prefix>/lib/cmake/mve/mveConfigVersion.cmake
install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Config
#   * <prefix>/lib/cmake/mve/mveTargets.cmake
install(
    EXPORT ${targets_export_name}
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)