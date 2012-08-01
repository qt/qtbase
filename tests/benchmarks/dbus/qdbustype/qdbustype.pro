TARGET = tst_bench_qdbustype
QT -= gui
QT += dbus dbus-private testlib
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
LIBS_PRIVATE += $$QT_LIBS_DBUS

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
