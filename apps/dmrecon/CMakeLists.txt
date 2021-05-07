add_executable(dmrecon dmrecon.cc fancy_progress_printer.cc)

target_link_libraries(dmrecon PUBLIC mve::mve_dmrecon mve::mve_mve mve::mve_util)

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(dmrecon PUBLIC Threads::Threads)
endif()

if(OPENMP_FOUND)
    target_link_libraries(dmrecon PUBLIC OpenMP::OpenMP_CXX)
endif()


install(TARGETS dmrecon DESTINATION ${CMAKE_INSTALL_BINDIR})