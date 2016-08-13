CONFIG += testcase
SOURCES += ../tst_qdbusmarshall.cpp
TARGET = ../tst_qdbusmarshall
DESTDIR = ./

QT = core-private dbus-private testlib

contains(QT_CONFIG, dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    QMAKE_USE += dbus
} else {
    SOURCES += ../../../../../src/dbus/qdbus_symbols.cpp
}
