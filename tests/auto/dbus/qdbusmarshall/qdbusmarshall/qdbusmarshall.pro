CONFIG += testcase
SOURCES += ../tst_qdbusmarshall.cpp
TARGET = ../tst_qdbusmarshall
DESTDIR = ./

QT = core-private dbus-private testlib

contains(QT_CONFIG, dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    LIBS += $$QMAKE_LIBS_DBUS
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_DBUS
} else {
    SOURCES += ../../../../../src/dbus/qdbus_symbols.cpp
}
