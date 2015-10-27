file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(meshalign ${SOURCES} ${HEADERS})
target_link_libraries(meshalign mve_fssr mve mve_util)
install(TARGETS meshalign RUNTIME DESTINATION .)
