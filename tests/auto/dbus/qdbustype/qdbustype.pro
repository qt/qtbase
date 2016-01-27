CONFIG += testcase parallel_test
TARGET = tst_qdbustype
QT = core-private dbus-private testlib
SOURCES += tst_qdbustype.cpp
contains(QT_CONFIG, dbus-linked) {
    DEFINES += QT_LINKED_LIBDBUS
    LIBS += $$QT_LIBS_DBUS
    QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
} else {
    SOURCES += ../../../../src/dbus/qdbus_symbols.cpp
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
