CONFIG += testcase
SOURCES += ../tst_qdbusinterface.cpp
HEADERS += ../myobject.h
TARGET = ../tst_qdbusinterface
DESTDIR = ./

QT = core core-private dbus dbus-private testlib

qtConfig(dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    QMAKE_USE += dbus
} else {
    SOURCES += ../../../../../src/dbus/qdbus_symbols.cpp
}
