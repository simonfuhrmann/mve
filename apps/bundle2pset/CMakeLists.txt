add_executable(bundle2pset bundle2pset.cc)

target_link_libraries(bundle2pset PUBLIC mve::mve_mve mve::mve_util)

install(TARGETS bundle2pset DESTINATION ${CMAKE_INSTALL_BINDIR})