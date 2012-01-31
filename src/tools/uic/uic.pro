TEMPLATE = app
TARGET = uic

DESTDIR = ../../../bin
DEFINES += QT_UIC
INCLUDEPATH += .
DEPENDPATH += .

include(uic.pri)
include(cpp/cpp.pri)

HEADERS += uic.h

SOURCES += main.cpp \
           uic.cpp

linux-g++-maemo:contains(QT_ARCH, arm) {
    # UIC will crash when running inside QEMU if built with -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
}

include(../bootstrap/bootstrap.pri)

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target
load(qt_targets)
