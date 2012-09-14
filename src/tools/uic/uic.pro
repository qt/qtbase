option(host_build)
TEMPLATE = app
TARGET = uic

DESTDIR = ../../../bin
DEFINES += QT_UIC

include(uic.pri)
include(cpp/cpp.pri)

HEADERS += uic.h

SOURCES += main.cpp \
           uic.cpp

*-maemo* {
    # UIC will crash when running inside QEMU if built with -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
}

include(../bootstrap/bootstrap.pri)

target.path = $$[QT_HOST_BINS]
INSTALLS += target
load(qt_targets)
