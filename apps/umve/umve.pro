MVE_ROOT = ../..

CONFIG += qt opengl release
QT += opengl

SOURCES += [^_]*.cc viewinspect/*.cc sceneinspect/*.cc
HEADERS += *.h viewinspect/*.h sceneinspect/*.h
RESOURCES = umve.qrc
TARGET = umve

INCLUDEPATH += $${MVE_ROOT}/libs
DEPENDPATH += $${MVE_ROOT}/libs
LIBS = $${MVE_ROOT}/libs/dmrecon/libmve_dmrecon.a $${MVE_ROOT}/libs/mve/libmve.a $${MVE_ROOT}/libs/ogl/libmve_ogl.a $${MVE_ROOT}/libs/util/libmve_util.a -lpng -ljpeg -ltiff -lGLEW
QMAKE_LIBDIR_QT =

OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build

exists(umve.priv.pro) : include(umve.priv.pro)
