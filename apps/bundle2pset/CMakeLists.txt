file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(bundle2pset ${SOURCES})
target_link_libraries(bundle2pset mve mve_util)
install(TARGETS bundle2pset RUNTIME DESTINATION .)
