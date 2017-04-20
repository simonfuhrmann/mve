file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(meshclean ${SOURCES} ${HEADERS})
target_link_libraries(meshclean mve_fssr mve mve_util)
install(TARGETS meshclean RUNTIME DESTINATION .)
