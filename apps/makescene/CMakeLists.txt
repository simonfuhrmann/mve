add_executable(makescene makescene.cc)

target_link_libraries(makescene PUBLIC mve::mve_math mve::mve_mve mve::mve_util)

install(TARGETS makescene DESTINATION bin)