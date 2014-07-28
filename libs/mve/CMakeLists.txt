file (GLOB HEADERS "*.h")
file (GLOB SOURCES "[^_]*.cc")

add_library(mve STATIC ${SOURCES})
set_target_properties(mve PROPERTIES DEBUG_POSTFIX ${MVE_LIBS_DEBUG_POSTFIX})
target_link_libraries(mve ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES})
