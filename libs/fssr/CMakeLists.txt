set(FSSR_HEADERS
	basis_function.h
	defines.h
	hermite.h
	iso_octree.h
	iso_surface.h
	mesh_clean.h
	octree.h
	sample.h
	sample_io.h
	triangulation.h
	voxel.h)

set(FSSR_SOURCES
	basis_function.cc
	hermite.cc
	iso_octree.cc
	iso_surface.cc
	mesh_clean.cc
	octree.cc
	sample_io.cc
	triangulation.cc
	voxel.cc)

add_library(mve_fssr STATIC ${FSSR_SOURCES})

target_include_directories(mve_fssr PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

set_target_properties(mve_fssr PROPERTIES VERSION ${PROJECT_VERSION})

target_link_libraries(mve_fssr PUBLIC mve_mve mve_math 
				PRIVATE mve_util)

# Targets:
#   * <prefix>/lib/libmve_ffsr.a
#   * header location after install: <prefix>/include/*/*.h
install(
    TARGETS mve_fssr
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * ffsr/*.h -> <prefix>/include/ffsr/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/fssr"
    DESTINATION "${include_install_dir}"
    FILES_MATCHING PATTERN "*.h"
)