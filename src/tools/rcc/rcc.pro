option(host_build)
TEMPLATE = app
TARGET = rcc
QT = bootstrap-private

DESTDIR = ../../../bin
DEFINES += QT_RCC QT_NO_CAST_FROM_ASCII

include(rcc.pri)
HEADERS += ../../corelib/kernel/qcorecmdlineargs_p.h
SOURCES += main.cpp

target.path = $$[QT_HOST_BINS]
INSTALLS += target
load(qt_targets)
