file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(sfmrecon ${SOURCES})
target_link_libraries(sfmrecon mve mve_sfm mve_util)
install(TARGETS sfmrecon RUNTIME DESTINATION .)
