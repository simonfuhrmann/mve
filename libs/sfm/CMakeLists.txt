file (GLOB HEADERS "*.h")
file (GLOB SOURCES "[^_]*.cc")

add_library(mve_sfm STATIC ${SOURCES})
set_target_properties(mve_sfm PROPERTIES DEBUG_POSTFIX ${MVE_LIBS_DEBUG_POSTFIX})
target_link_libraries(mve_sfm mve)

