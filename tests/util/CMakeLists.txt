set(MVE_UTIL_TEST_SOURCES
        aligned_memory
        endian
        file_system
        string
        tokenizer
        )

foreach(testname IN LISTS MVE_UTIL_TEST_SOURCES)
    set(test_target smvs_test_util_${testname})
    add_executable(${test_target} gtest_${testname}.cc)
    target_link_libraries(${test_target} GTest::gtest GTest::gtest_main mve_util)

    add_test(NAME ${test_target} COMMAND ${test_target})
    unset(test_target)
endforeach()