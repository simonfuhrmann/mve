
file (GLOB UMVE_HEADERS "*.h" "viewinspect/*.h" "scene_inspect/*.h" "scene_addins/*.h")
file (GLOB UMVE_SOURCES "*.cc" "viewinspect/*.cc" "scene_inspect/*.cc" "scene_addins/*.cc")

find_package(Qt5 REQUIRED Core Gui OpenGL Svg Widgets)
find_package(OpenGL REQUIRED)

qt5_wrap_cpp(HEADERS_MOC ${UMVE_HEADERS})
qt5_add_resources(RESOURCES_RCC umve.qrc)

add_executable(umve ${UMVE_SOURCES} ${HEADERS_MOC} ${RESOURCES_RCC})

target_include_directories(umve PUBLIC ${OPENGL_INCLUDE_DIR}
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>")

# needed because otherwise somehow opengl is imported without  GL_GLEXT_PROTOTYPES
target_compile_definitions(umve PUBLIC GL_GLEXT_PROTOTYPES)

set_target_properties(umve PROPERTIES ENABLE_EXPORTS TRUE)

target_link_libraries(umve PUBLIC Qt5::Core
	Qt5::Gui
	Qt5::OpenGL
	Qt5::Svg
	Qt5::Widgets
        ${OPENGL_LIBRARIES}
        mve::mve_dmrecon
        mve::mve_ogl
        mve::mve_util
        mve::mve_mve
        mve::mve_sfm)


install(TARGETS umve DESTINATION bin)
install(DIRECTORY shaders DESTINATION bin)