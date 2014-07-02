find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
set(MVE_OGL_LIBS ${OPENGL_LIBRARIES})

if(WIN32)
	find_package(GLEW REQUIRED)
	include_directories(${GLEW_INCLUDE_DIRS})
	set(MVE_OGL_LIBS ${MVE_OGL_LIBS} ${GLEW_LIBRARIES})
endif()

file (GLOB HEADERS "*.h")
file (GLOB SOURCES "[^_]*.cc")

add_library(mve_ogl STATIC ${SOURCES})

target_link_libraries(mve_ogl
	${MVE_OGL_LIBS}
)

