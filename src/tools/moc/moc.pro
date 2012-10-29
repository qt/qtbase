option(host_build)
TEMPLATE = app
TARGET = moc
QT = bootstrap-private

DEFINES += QT_MOC QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_COMPRESS
DESTDIR = ../../../bin

INCLUDEPATH += $$QT_BUILD_TREE/src/corelib/global

include(moc.pri)
HEADERS += qdatetime_p.h
SOURCES += main.cpp

target.path = $$[QT_HOST_BINS]
INSTALLS += target
load(qt_targets)
