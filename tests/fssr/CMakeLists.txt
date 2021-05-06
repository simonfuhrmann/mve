set(MVE_FSSR_TEST_SOURCES
        iso_octree
        mesh_clean
        octree
        octree_iterator
        )

foreach(testname IN LISTS MVE_FSSR_TEST_SOURCES)
    set(test_target smvs_test_fssr_${testname})
    add_executable(${test_target} gtest_${testname}.cc)
    target_link_libraries(${test_target} GTest::gtest GTest::gtest_main mve_fssr)

    add_test(NAME ${test_target} COMMAND ${test_target})
    unset(test_target)
endforeach()