option(host_build)
TEMPLATE	= app
TARGET		= moc

DEFINES         += QT_MOC QT_NO_CAST_FROM_BYTEARRAY QT_NO_COMPRESS
DESTDIR         = ../../../bin
INCLUDEPATH	+= . $$OUT_PWD/../../corelib/global
DEPENDPATH	+= .
LIBS	        =
OBJECTS_DIR	= .

include(moc.pri)
HEADERS += qdatetime_p.h
SOURCES += main.cpp
include(../bootstrap/bootstrap.pri)

target.path = $$[QT_HOST_BINS]
INSTALLS += target
load(qt_targets)
