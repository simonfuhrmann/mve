set(MVE_MATH_TEST_SOURCES
        accum
        algo
        defines
        functions
        geom
        matrix
        matrix_qr
        matrix_svd
        matrix_tools
        vector
        )

foreach(testname IN LISTS MVE_MATH_TEST_SOURCES)
    set(test_target smvs_test_math_${testname})
    add_executable(${test_target} gtest_${testname}.cc)
    target_link_libraries(${test_target} GTest::GTest GTest::Main mve_math)

    add_test(NAME ${test_target} COMMAND ${test_target})
    unset(test_target)
endforeach()