QT =
CONFIG = debug

SOURCES = [^_]*.cc _test.cc
HEADERS = *.h
TARGET = test

DEFINES += MVE_NO_TIFF_SUPPORT MVE_NO_JPEG_SUPPORT
INCLUDEPATH += ..
LIBS = -L../util -lutil -lpng

