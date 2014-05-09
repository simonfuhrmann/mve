file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(meshconvert ${SOURCES})
target_link_libraries(meshconvert mve mve_util)
install(TARGETS meshconvert RUNTIME DESTINATION .)
