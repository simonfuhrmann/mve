file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(mesh2pset ${SOURCES})
target_link_libraries(mesh2pset mve_fssr mve mve_util)
install(TARGETS mesh2pset RUNTIME DESTINATION .)
