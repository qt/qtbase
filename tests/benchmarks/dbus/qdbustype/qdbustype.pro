TARGET = tst_bench_qdbustype
QT -= gui
QT += core-private dbus-private testlib
contains(QT_CONFIG, dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    LIBS += $$QT_LIBS_DBUS
    QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
} else {
    SOURCES += ../../../../src/dbus/qdbus_symbols.cpp
}

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
