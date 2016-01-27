CONFIG += testcase parallel_test
TARGET = tst_qdbusconnection_no_libdbus
QT = core dbus testlib
DEFINES += SIMULATE_LOAD_FAIL
SOURCES += ../qdbusconnection_no_bus/tst_qdbusconnection_no_bus.cpp
