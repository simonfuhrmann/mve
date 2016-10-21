add_executable(dmrecon dmrecon.cc fancy_progress_printer.cc)

target_link_libraries(dmrecon PUBLIC mve::mve_dmrecon mve::mve_mve mve::mve_util)

install(TARGETS dmrecon DESTINATION bin)