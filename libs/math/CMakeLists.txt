set(MATH_HEADERS
	accum.h
	algo.h
	bezier_curve.h
	bspline.h
	defines.h
	functions.h
	geometry.h
	line.h
	matrix.h
	matrix_qr.h
	matrix_svd.h
	matrix_tools.h
	octree_tools.h
	permute.h
	plane.h
	quaternion.h
	vector.h)


add_library(mve_math INTERFACE)

# target_sources(mve_math INTERFACE ${MATH_HEADERS})

set_target_properties(mve_mve PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mve_util PROPERTIES DEBUG_POSTFIX "d")

target_include_directories(mve_math INTERFACE
    	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs>"
		# this to be able to include as <mve/math/...>, recommended
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
		# this to be able to include as <math/...>, not recommended
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/mve")


# Targets:
#   * <prefix>/lib/libmve_math.a
#   * header location after install: <prefix>/include/math/*.h
install(
    TARGETS mve_math
    EXPORT "${targets_export_name}"
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * math/*.h -> <prefix>/include/math/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/libs/math"
    DESTINATION "${include_install_dir}/mve"
    FILES_MATCHING PATTERN "*.h"
)