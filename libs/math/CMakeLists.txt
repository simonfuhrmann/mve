file (GLOB HEADERS "*.h")
file (GLOB SOURCES "[^_]*.cc")

# FIXME: This is a hack to get the headers of mve_math to show up in the IDE.
#        In the future, this will be replaced with something else when CMake
#        makes header only libraries/targets first class citizens.
add_custom_target(mve_math SOURCES ${SOURCES} ${HEADERS})
