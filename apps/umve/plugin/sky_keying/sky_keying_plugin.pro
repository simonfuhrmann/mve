TEMPLATE = lib

MVE_ROOT = ../../../../

CONFIG += qt opengl plugin c++11
QT += opengl

QMAKE_CXXFLAGS += -fPIC -fopenmp -std=c++11 -O3
QMAKE_LFLAGS += -rdynamic

SOURCES += [^_]*.cc
HEADERS += *.h
TARGET = ../$$qtLibraryTarget(SkyKeyingTab)

INCLUDEPATH += $${MVE_ROOT}/libs ../..
DEPENDPATH += $${MVE_ROOT}/libs
LIBS = $${MVE_ROOT}/libs/dmrecon/libmve_dmrecon.a $${MVE_ROOT}/libs/mve/libmve.a $${MVE_ROOT}/libs/ogl/libmve_ogl.a $${MVE_ROOT}/libs/util/libmve_util.a -lpng -ljpeg -ltiff -lGLEW -fopenmp
QMAKE_LIBDIR_QT =

OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

