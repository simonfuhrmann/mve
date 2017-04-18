file (GLOB SOURCES "[^_]*.cc")

# Add test cpp file
add_executable(util_test ${SOURCES})

# Link test executable against gtest & gtest_main
target_link_libraries(util_test ${GTEST_LIBRARY_DEBUG} ${GTEST_MAIN_LIBRARY_DEBUG} mve mve_util)

add_test(NAME util_test COMMAND util_test)
