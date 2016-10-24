set(SFM_HEADERS
        bundler_common.h
        bundler_features.h
        bundler_incremental.h
        bundler_init_pair.h
        bundler_intrinsics.h
        bundler_matching.h
        bundler_tracks.h
        camera_database.h
        camera_pose.h
        cascade_hashing.h
        correspondence.h
        defines.h
        exhaustive_matching.h
        extract_focal_length.h
        feature_set.h
        fundamental.h
        homography.h
        matching.h
        matching_base.h
        nearest_neighbor.h
        pba_config.h
        pba_cpu.h
        pba_types.h
        pose_p3p.h
        ransac.h
        ransac_fundamental.h
        ransac_homography.h
        ransac_pose_p3p.h
        sift.h
        surf.h
        triangulate.h
        visualizer.h)

set(SFM_SOURCES
	bundler_common.cc
        bundler_features.cc
        bundler_incremental.cc
        bundler_init_pair.cc
        bundler_intrinsics.cc
        bundler_matching.cc
        bundler_tracks.cc
        camera_database.cc
        cascade_hashing.cc
        exhaustive_matching.cc
        extract_focal_length.cc
        feature_set.cc
        fundamental.cc
        homography.cc
        matching.cc
        nearest_neighbor.cc
        pba_config.cc
        pba_cpu.cc
        pose_p3p.cc
        ransac.cc
        ransac_fundamental.cc
        ransac_homography.cc
        ransac_pose_p3p.cc
        sift.cc
        surf.cc
        triangulate.cc
        visualizer.cc)

add_library(mve_sfm STATIC ${SFM_SOURCES})

target_include_directories(mve_sfm PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

target_compile_options(mve_sfm PUBLIC -msse2 -msse3 -mpopcnt)

if(OPENMP_FOUND)
   target_compile_options(mve_sfm PUBLIC "${OpenMP_CXX_FLAGS}")
   target_link_libraries(mve_sfm PUBLIC ${OpenMP_CXX_FLAGS})
endif()

set_target_properties(mve_sfm PROPERTIES VERSION ${PROJECT_VERSION})

target_link_libraries(mve_sfm PUBLIC mve_mve mve_math mve_util)

# Targets:
#   * <prefix>/lib/libmve_sfm.a
#   * header location after install: <prefix>/include/*/*.h
install(
    TARGETS mve_sfm
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * sfm/*.h -> <prefix>/include/sfm/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/sfm"
    DESTINATION "${include_install_dir}"
    FILES_MATCHING PATTERN "*.h"
)
