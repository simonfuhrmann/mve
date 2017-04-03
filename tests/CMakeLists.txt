enable_testing()
find_package(Gtest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_subdirectory(math)
add_subdirectory(util)
add_subdirectory(mve)
