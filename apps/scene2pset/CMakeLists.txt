add_executable(scene2pset scene2pset.cc)

target_link_libraries(scene2pset PUBLIC mve::mve_mve mve::mve_util mve::mve_math)

install(TARGETS scene2pset DESTINATION ${CMAKE_INSTALL_BINDIR})