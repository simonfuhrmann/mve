set(UTIL_HEADERS
	aligned_allocator.h
	aligned_memory.h
	arguments.h
	defines.h
	endian.h
	exception.h
	file_system.h
	frame_timer.h
	ini_parser.h
	string.h
	system.h
	timer.h
	tokenizer.h)

set(UTIL_SOURCES
	arguments.cc
	file_system.cc
	ini_parser.cc
	system.cc)

add_library(mve_util ${UTIL_SOURCES})

target_include_directories(mve_util PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs>"
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mve>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>)

set_target_properties(mve_util PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mve_util PROPERTIES DEBUG_POSTFIX "d")

# Targets:
#   * <prefix>/lib/libmve_util.a
#   * header location after install: <prefix>/include/*/*.h
install(
    TARGETS mve_util
    EXPORT ${targets_export_name}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * util/*.h -> <prefix>/include/util/*.h
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/libs/util"
    DESTINATION "${include_install_dir}/mve"
    FILES_MATCHING PATTERN "*.h"
)