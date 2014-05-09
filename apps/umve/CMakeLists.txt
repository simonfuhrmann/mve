file (GLOB HEADERS "*.h" "viewinspect/*.h" "scene_inspect/*.h" "scene_addins/*.h")
file (GLOB SOURCES "*.cc" "viewinspect/*.cc" "scene_inspect/*.cc" "scene_addins/*.cc")

set(UMVE_QT_COMPONENTS Core Gui OpenGL Svg Widgets)
find_package(Qt5 5.2.0 REQUIRED ${UMVE_QT_COMPONENTS})
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
include_directories(. ${GLEW_INCLUDE_DIRS})

qt5_wrap_cpp(HEADERS_MOC ${HEADERS})
qt5_add_resources(RESOURCES_RCC umve.qrc)

add_executable(umve ${SOURCES} ${HEADERS_MOC} ${RESOURCES_RCC})
set_target_properties(umve PROPERTIES ENABLE_EXPORTS TRUE)
target_link_libraries(umve
	Qt5::Core
	Qt5::Gui
	Qt5::OpenGL
	Qt5::Svg
	Qt5::Widgets
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    mve_dmrecon mve mve_ogl mve_util
)

add_subdirectory(plugins)

if(WIN32)
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    if(MSVC)
        # HACK: Install Visual Studio's OpenMP library
        if(MSVC11)
            set(MSVC_VER "11")
        elseif(MSVC12)
            set(MSVC_VER "12")
        else()
            message(FATAL_ERROR "Unsupported Visual Studio version")
        endif()

        if(CMAKE_CL_64)
            set(CMAKE_MSVC_ARCH x64)
        else()
            set(CMAKE_MSVC_ARCH x86)
        endif()

        set(OpenMP_REDIST_LIB "${MSVC${MSVC_VER}_REDIST_DIR}/${CMAKE_MSVC_ARCH}/Microsoft.VC${MSVC_VER}0.OpenMP/vcomp${MSVC_VER}0.dll")
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${OpenMP_REDIST_LIB})
        include(InstallRequiredSystemLibraries)
    endif()

    # Install Qt5 DLLs
	set(QT5_BASE_DIR "${_qt5Core_install_prefix}")
    foreach(component ${UMVE_QT_COMPONENTS})
		set(component_DLL_RELEASE ${QT5_BASE_DIR}/bin/Qt5${component}.dll)
		set(component_DLL_DEBUG ${QT5_BASE_DIR}/bin/Qt5${component}d.dll)
        install(FILES ${component_DLL_RELEASE}
            DESTINATION .
            CONFIGURATIONS MinSizeRel Release)
        install(FILES ${component_DLL_DEBUG}
            DESTINATION .
            CONFIGURATIONS Debug RelWithDebInfo)
    endforeach()

	# Install Qt5 platform DLL
	install(FILES "${QT5_BASE_DIR}/plugins/platforms/qwindows.dll"
		DESTINATION qt_plugins/platforms
		CONFIGURATIONS MinSizeRel Release)
	install(FILES "${QT5_BASE_DIR}/plugins/platforms/qwindowsd.dll"
		DESTINATION qt_plugins/platforms
		CONFIGURATIONS Debug RelWithDebInfo)

	# Install SVG imageformat DLL
	install(FILES "${QT5_BASE_DIR}/plugins/imageformats/qsvg.dll"
		DESTINATION qt_plugins/imageformats
		CONFIGURATIONS MinSizeRel Release)
	install(FILES "${QT5_BASE_DIR}/plugins/imageformats/qsvgd.dll"
		DESTINATION qt_plugins/imageformats
		CONFIGURATIONS Debug RelWithDebInfo)
endif()
install(DIRECTORY shaders DESTINATION .)
install(TARGETS umve RUNTIME DESTINATION .)
