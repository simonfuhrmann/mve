add_executable(mesh2pset mesh2pset.cc)

target_link_libraries(mesh2pset PUBLIC mve::mve_mve mve::mve_util)

install(TARGETS mesh2pset DESTINATION bin)