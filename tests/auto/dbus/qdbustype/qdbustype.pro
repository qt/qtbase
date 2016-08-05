CONFIG += testcase parallel_test
TARGET = tst_qdbustype
QT = core-private dbus-private testlib
SOURCES += tst_qdbustype.cpp
qtConfig(dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    QMAKE_USE += dbus
} else {
    SOURCES += ../../../../src/dbus/qdbus_symbols.cpp
}
