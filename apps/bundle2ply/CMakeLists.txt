file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(bundle2ply ${SOURCES})
target_link_libraries(bundle2ply mve mve_util)
install(TARGETS bundle2ply RUNTIME DESTINATION .)
