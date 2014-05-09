file (GLOB HEADERS "*.h")
file (GLOB SOURCES "*.cc")

qt5_wrap_cpp(EXAMPLE_HEADERS_MOC ${HEADERS})

add_library(example MODULE ${SOURCES} ${EXAMPLE_HEADERS_MOC})
set_target_properties(example PROPERTIES DEFINE_SYMBOL "COMPILING_PLUGIN")
target_link_libraries(example
	Qt5::Core
	Qt5::Gui
	Qt5::OpenGL
	Qt5::Widgets
	umve
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    mve_dmrecon mve mve_ogl mve_util
)

install(TARGETS example LIBRARY DESTINATION plugins)
