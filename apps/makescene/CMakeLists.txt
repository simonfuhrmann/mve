add_executable(makescene makescene.cc)

target_link_libraries(makescene PUBLIC mve::mve_math mve::mve_mve mve::mve_util)

if(OPENMP_FOUND)
    target_link_libraries(makescene PUBLIC OpenMP::OpenMP_CXX)
endif()

install(TARGETS makescene DESTINATION ${CMAKE_INSTALL_BINDIR})
