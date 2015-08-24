file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

add_executable(sceneupgrade ${SOURCES})
target_link_libraries(sceneupgrade mve mve_sfm mve_util)
install(TARGETS sceneupgrade RUNTIME DESTINATION .)
