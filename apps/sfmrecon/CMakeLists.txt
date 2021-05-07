add_executable(sfmrecon sfmrecon.cc)

target_link_libraries(sfmrecon PUBLIC mve::mve_mve mve::mve_util mve::mve_sfm)

if(OPENMP_FOUND)
    target_link_libraries(sfmrecon PUBLIC OpenMP::OpenMP_CXX)
endif()

install(TARGETS sfmrecon DESTINATION ${CMAKE_INSTALL_BINDIR})