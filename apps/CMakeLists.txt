if(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  # I am top-level project, i.e. I am not being include by another project
  cmake_minimum_required(VERSION 3.13)
  project(mveApps LANGUAGES CXX)

  option(BUILD_SHARED_LIBS "Build shared libraries" ON)

  include(GNUInstallDirs)

  set(CMAKE_POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

endif()

# create alias if it is included
if(TARGET mve_util)
  add_library(mve::mve_util ALIAS mve_util)
  add_library(mve::mve_mve ALIAS mve_mve)
  add_library(mve::mve_ogl ALIAS mve_ogl)
  add_library(mve::mve_fssr ALIAS mve_fssr)
  add_library(mve::mve_dmrecon ALIAS mve_dmrecon)
  add_library(mve::mve_math ALIAS mve_math)
  add_library(mve::mve_sfm ALIAS mve_sfm)
else()
  find_package(mve CONFIG REQUIRED)
endif()

add_subdirectory(bundle2pset)
add_subdirectory(dmrecon)
add_subdirectory(fssrecon)
add_subdirectory(makescene)
add_subdirectory(mesh2pset)
add_subdirectory(meshalign)
add_subdirectory(meshclean)
add_subdirectory(meshconvert)
add_subdirectory(prebundle)
add_subdirectory(scene2pset)
add_subdirectory(sceneupgrade)
add_subdirectory(sfmrecon)
if(MVE_BUILD_UMVE)
  add_subdirectory(umve)
endif()