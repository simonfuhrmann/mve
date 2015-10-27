file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(scene2pset ${SOURCES} ${HEADERS})
target_link_libraries(scene2pset mve mve_util)
install(TARGETS scene2pset RUNTIME DESTINATION .)
