file (GLOB HEADERS "*.h")
file (GLOB SOURCES "[^_]*.cc")

add_library(mve_util STATIC ${SOURCES})
set_target_properties(mve_util PROPERTIES DEBUG_POSTFIX ${MVE_LIBS_DEBUG_POSTFIX})
