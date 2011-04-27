load(qttest_p4)
TARGET = tst_bench_qdbustype
QT -= gui
QT += dbus
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
LIBS_PRIVATE += $$QT_LIBS_DBUS

SOURCES += main.cpp
