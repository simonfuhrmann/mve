file (GLOB SOURCES "[^_]*.cc")

# Add test cpp file
add_executable(math_test ${SOURCES})

# Link test executable against gtest & gtest_main
target_link_libraries(math_test ${GTEST_LIBRARY_DEBUG} ${GTEST_MAIN_LIBRARY_DEBUG} mve)

add_test(NAME math_test COMMAND math_test)