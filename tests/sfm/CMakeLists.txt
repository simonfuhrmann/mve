set(MVE_SFM_TEST_SOURCES
        bundler_tracks
        homography
        matching
        nearest_neighbor
        pose
        pose_p3p
        surf
        )

foreach(testname IN LISTS MVE_SFM_TEST_SOURCES)
    set(test_target smvs_test_sfm_${testname})
    add_executable(${test_target} gtest_${testname}.cc)
    target_link_libraries(${test_target} GTest::gtest GTest::gtest_main mve_sfm)

    add_test(NAME ${test_target} COMMAND ${test_target})
    unset(test_target)
endforeach()