MVE_ROOT = ../..

CONFIG += qt opengl release
QT += opengl

QMAKE_LFLAGS += -rdynamic -fopenmp
QMAKE_CXXFLAGS += -fPIC -fopenmp

SOURCES += [^_]*.cc viewinspect/*.cc scene_inspect/*.cc scene_addins/*.cc sfm_reconstruct/*.cc
HEADERS += *.h viewinspect/*.h scene_inspect/*.h scene_addins/*.h sfm_reconstruct/*.h
RESOURCES = umve.qrc
TARGET = umve

INCLUDEPATH += $${MVE_ROOT}/libs
DEPENDPATH += $${MVE_ROOT}/libs
LIBS = $${MVE_ROOT}/libs/sfm/libmve_sfm.a $${MVE_ROOT}/libs/dmrecon/libmve_dmrecon.a $${MVE_ROOT}/libs/mve/libmve.a $${MVE_ROOT}/libs/ogl/libmve_ogl.a $${MVE_ROOT}/libs/util/libmve_util.a -lpng -ljpeg -ltiff
QMAKE_LIBDIR_QT =

OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build

# Options specific to OS X.
macx {
    CONFIG -= app_bundle
    QMAKE_LFLAGS += -L/usr/local/lib
    QMAKE_LFLAGS -= -fopenmp
    INCLUDEPATH += /usr/local/include
    QMAKE_CXXFLAGS -= -fopenmp
}

# Options specific to Windows.
win32 {
    LIBS += -lGLEW
}

exists(umve.priv.pro) : include(umve.priv.pro)
