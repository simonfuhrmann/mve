set(OGL_HEADERS
        camera.h
        camera_2d.h
        camera_trackball.h
        check_gl_error.h
        context.h
        defines.h
        events.h
        key_symbols.h
        mesh_renderer.h
        opengl.h
        render_tools.h
        shader_program.h
        texture.h
        vertex_array.h
        vertex_buffer.h)

set(OGL_SOURCES
        camera_2d.cc
        camera_trackball.cc
        events.cc
        mesh_renderer.cc
        render_tools.cc
        shader_program.cc
        texture.cc
        vertex_array.cc
        vertex_buffer.cc)

find_package(OpenGL REQUIRED)

if(WIN32)
    find_package(GLEW REQUIRED)
endif()

add_library(mve_ogl ${OGL_SOURCES})

target_include_directories(mve_ogl PUBLIC ${OPENGL_INCLUDE_DIR}
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs>"
        # this to be able to include as <mve/ogl/...>, recommended
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        # this to be able to include as <ogl/...>, not recommended
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/mve")

if(WIN32)
    target_include_directories(mve_ogl PUBLIC ${GLEW_INCLUDE_DIRS})
endif()

set_target_properties(mve_ogl PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mve_util PROPERTIES DEBUG_POSTFIX "d")

target_link_libraries(mve_ogl PUBLIC mve_mve mve_math mve_util ${OPENGL_LIBRARIES})
if(WIN32)
    target_link_libraries(mve_ogl PUBLIC ${GLEW_LIBRARIES})
endif()

# Targets:
#   * <prefix>/lib/libmve_ogl.a
#   * header location after install: <prefix>/include/ogl/*.h
install(
    TARGETS mve_ogl
    EXPORT "${targets_export_name}"
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * ogl/*.h -> <prefix>/include/ogl/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ogl"
    DESTINATION "${include_install_dir}/mve"
    FILES_MATCHING PATTERN "*.h"
)