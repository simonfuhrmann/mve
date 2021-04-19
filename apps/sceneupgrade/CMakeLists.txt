add_executable(sceneupgrade sceneupgrade.cc)

target_link_libraries(sceneupgrade PUBLIC mve::mve_mve mve::mve_util mve::mve_sfm)

install(TARGETS sceneupgrade DESTINATION ${CMAKE_INSTALL_BINDIR})