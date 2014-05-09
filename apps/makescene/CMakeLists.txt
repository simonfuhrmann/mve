file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(makescene ${SOURCES})
target_link_libraries(makescene mve mve_util)
install(TARGETS makescene RUNTIME DESTINATION .)
