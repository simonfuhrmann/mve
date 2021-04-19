add_executable(prebundle prebundle.cc)

target_link_libraries(prebundle PUBLIC mve::mve_sfm mve::mve_util)

install(TARGETS prebundle DESTINATION ${CMAKE_INSTALL_BINDIR})