find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIRS}
)

file (GLOB HEADERS "*.h")
file (GLOB SOURCES "[^_]*.cc")

add_library(mve_ogl STATIC ${SOURCES})

target_link_libraries(mve_ogl
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
)

