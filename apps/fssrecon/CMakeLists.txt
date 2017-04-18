file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(fssrecon ${SOURCES} ${HEADERS})
target_link_libraries(fssrecon mve_fssr mve mve_util)
install(TARGETS fssrecon RUNTIME DESTINATION .)
