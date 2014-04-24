TEMPLATE = lib

MVE_ROOT = ../../../../

CONFIG += qt opengl plugin
QT += opengl

QMAKE_CXXFLAGS += -fPIC
QMAKE_LFLAGS += -rdynamic

SOURCES += [^_]*.cc
HEADERS += *.h
TARGET = $$qtLibraryTarget(HelloWorldTab)

target.path = $$(HOME)/.local/share/umve/plugins/
INSTALLS += target

INCLUDEPATH += $${MVE_ROOT}/libs ../..
DEPENDPATH += $${MVE_ROOT}/libs
LIBS = $${MVE_ROOT}/libs/dmrecon/libmve_dmrecon.a $${MVE_ROOT}/libs/mve/libmve.a $${MVE_ROOT}/libs/ogl/libmve_ogl.a $${MVE_ROOT}/libs/util/libmve_util.a -lpng -ljpeg -ltiff -lGLEW
QMAKE_LIBDIR_QT =

OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build
