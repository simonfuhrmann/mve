add_executable(fssrecon fssrecon.cc)

target_link_libraries(fssrecon PUBLIC mve::mve_fssr mve::mve_mve mve::mve_util)

install(TARGETS fssrecon DESTINATION bin)