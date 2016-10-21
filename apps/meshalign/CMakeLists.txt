add_executable(meshalign meshalign.cc meshlab_alignment.cc stanford_alignment.cc)

target_link_libraries(meshalign PUBLIC mve::mve_mve mve::mve_util mve::mve_math)

install(TARGETS meshalign DESTINATION bin)