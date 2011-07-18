load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_bench_qdbustype
QT -= gui
QT += dbus dbus-private
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
LIBS_PRIVATE += $$QT_LIBS_DBUS

SOURCES += main.cpp
