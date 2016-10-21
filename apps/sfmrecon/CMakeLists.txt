add_executable(sfmrecon sfmrecon.cc)

target_link_libraries(sfmrecon PUBLIC mve::mve_mve mve::mve_util mve::mve_sfm)

install(TARGETS sfmrecon DESTINATION bin)