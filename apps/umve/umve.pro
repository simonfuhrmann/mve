CONFIG += qt opengl release
QT += opengl

SOURCES += [^_]*.cc viewinspect/*.cc sceneinspect/*.cc
HEADERS += *.h viewinspect/*.h sceneinspect/*.h
RESOURCES = umve.qrc
TARGET = umve
TARGET.path = ../../bin
INSTALLS += TARGET

INCLUDEPATH += ../../libs ../../extern
DEPENDPATH += ../../libs
LIBS = -L../../libs/mve -L../../libs/ogl -L../../libs/util -L../../libs/dmrecon -ldmrecon -lmve -logl -lutil -lpng -ljpeg -ltiff -lGLEW
QMAKE_LIBDIR_QT =

OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build

exists(umve.priv.pro) : include(umve.priv.pro)
