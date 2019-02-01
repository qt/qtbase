TEMPLATE = app

debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../debug/helper
    } else {
        TARGET = ../release/helper
    }
} else {
    TARGET = ../helper
}

QT = core

DESTDIR = ./

CONFIG += cmdline

SOURCES += main.cpp
DEFINES += QT_MESSAGELOGCONTEXT

gcc:!mingw:!haiku {
    QMAKE_LFLAGS += -rdynamic
    contains(QT_ARCH, arm): QMAKE_CXXFLAGS += -funwind-tables -fno-inline
}
