set(DMRECON_HEADERS
        defines.h
        dmrecon.h
        global_view_selection.h
        image_pyramid.h
        local_view_selection.h
        mvs_tools.h
        patch_optimization.h
        patch_sampler.h
        progress.h
        settings.h
        single_view.h
        view_selection.h)

set(DMRECON_SOURCES
        dmrecon.cc
        global_view_selection.cc
        image_pyramid.cc
        local_view_selection.cc
        mvs_tools.cc
        patch_optimization.cc
        patch_sampler.cc
        single_view.cc)

add_library(mve_dmrecon STATIC ${DMRECON_SOURCES})

target_include_directories(mve_dmrecon PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

target_compile_options(mve_dmrecon PUBLIC -msse2 -msse3 -mpopcnt)

set_target_properties(mve_dmrecon PROPERTIES VERSION ${PROJECT_VERSION})

target_link_libraries(mve_dmrecon PUBLIC mve_mve mve_math PRIVATE mve_util)

# Targets:
#   * <prefix>/lib/libmve_dmrecon.a
#   * header location after install: <prefix>/include/*/*.h
install(
    TARGETS mve_dmrecon
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * dmrecon/*.h -> <prefix>/include/dmrecon/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/dmrecon"
    DESTINATION "${include_install_dir}"
    FILES_MATCHING PATTERN "*.h"
)
