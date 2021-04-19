add_executable(meshclean meshclean.cc)

target_link_libraries(meshclean PUBLIC mve::mve_fssr mve::mve_mve mve::mve_util)

install(TARGETS meshclean DESTINATION ${CMAKE_INSTALL_BINDIR})