option(host_build)
TEMPLATE = app
TARGET = rcc

DESTDIR = ../../../bin
DEFINES += QT_RCC

include(rcc.pri)
HEADERS += ../../corelib/kernel/qcorecmdlineargs_p.h
SOURCES += main.cpp
include(../bootstrap/bootstrap.pri)

target.path = $$[QT_HOST_BINS]
INSTALLS += target
load(qt_targets)
