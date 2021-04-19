set(MVE_HEADERS
	bundle.h
	bundle_io.h
	camera.h
	defines.h
	depthmap.h
	image.h
	image_base.h
	image_color.h
	image_drawing.h
	image_exif.h
	image_io.h
	image_tools.h
	marching_cubes.h
	marching_tets.h
	mesh.h
	mesh_info.h
	mesh_io.h
	mesh_io_npts.h
	mesh_io_obj.h
	mesh_io_off.h
	mesh_io_pbrt.h
	mesh_io_ply.h
	mesh_io_smf.h
	mesh_tools.h
	scene.h
	view.h
	volume.h)

set(MVE_SOURCES
	bundle.cc
	bundle_io.cc
	camera.cc
	depthmap.cc
	image_exif.cc
	image_io.cc
	image_tools.cc
	marching.cc
	mesh.cc
	mesh_info.cc
	mesh_io.cc
	mesh_io_npts.cc
	mesh_io_obj.cc
	mesh_io_off.cc
	mesh_io_pbrt.cc
	mesh_io_ply.cc
	mesh_io_smf.cc
	mesh_tools.cc
	scene.cc
	view.cc
	volume.cc)

add_library(mve_mve ${MVE_SOURCES})

target_include_directories(mve_mve PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

if(NOT MVE_NO_PNG_SUPPORT)
	target_include_directories(mve_mve PRIVATE ${PNG_INCLUDE_DIRS})
endif()
if(NOT MVE_NO_JPEG_SUPPORT)
	target_include_directories(mve_mve PRIVATE ${JPEG_INCLUDE_DIR})
endif()
if(NOT MVE_NO_TIFF_SUPPORT)
	target_include_directories(mve_mve PRIVATE ${TIFF_INCLUDE_DIR})
endif()

set_target_properties(mve_mve PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mve_util PROPERTIES DEBUG_POSTFIX "d")

# set_target_properties(mve_mve PROPERTIES
#     CXX_STANDARD 11
#     CXX_STANDARD_REQUIRED YES
# )

target_link_libraries(mve_mve PUBLIC mve_util mve_math)
if(NOT MVE_NO_PNG_SUPPORT)
    target_link_libraries(mve_mve PRIVATE ${PNG_LIBRARIES})
endif()
if(NOT MVE_NO_JPEG_SUPPORT)
    target_link_libraries(mve_mve PRIVATE ${JPEG_LIBRARIES})
endif()
if(NOT MVE_NO_TIFF_SUPPORT)
    target_link_libraries(mve_mve PRIVATE ${TIFF_LIBRARIES})
endif()

if(NOT MVE_NO_PNG_SUPPORT)
    target_compile_definitions(mve_mve PRIVATE ${PNG_DEFINITIONS})
else()
    target_compile_definitions(mve_mve PRIVATE "-DMVE_NO_PNG_SUPPORT")
endif()

if(MVE_NO_JPEG_SUPPORT)
    target_compile_definitions(mve_mve PRIVATE "-DMVE_NO_JPEG_SUPPORT")
endif()

if(MVE_NO_TIFF_SUPPORT)
    target_compile_definitions(mve_mve PRIVATE "-DMVE_NO_TIFF_SUPPORT")
endif()

# Targets:
#   * <prefix>/lib/libmve_mve.a
#   * header location after install: <prefix>/include/*/*.h
install(
    TARGETS mve_mve
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * mve/*.h -> <prefix>/include/mve/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/mve"
    DESTINATION "${include_install_dir}"
    FILES_MATCHING PATTERN "*.h"
)