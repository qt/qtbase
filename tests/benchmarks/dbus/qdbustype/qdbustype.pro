TARGET = tst_bench_qdbustype
QT -= gui
QT += core-private dbus-private testlib
qtConfig(dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    QMAKE_USE += dbus
} else {
    SOURCES += ../../../../src/dbus/qdbus_symbols.cpp
}

SOURCES += main.cpp
