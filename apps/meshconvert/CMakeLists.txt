add_executable(meshconvert meshconvert.cc)

target_link_libraries(meshconvert PUBLIC mve::mve_mve mve::mve_util)

install(TARGETS meshconvert DESTINATION ${CMAKE_INSTALL_BINDIR})