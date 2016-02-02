CONFIG += testcase parallel_test
TARGET = tst_qdbusconnection_no_libdbus
QT = core dbus testlib
DEFINES += SIMULATE_LOAD_FAIL tst_QDBusConnectionNoBus=tst_QDBusConnectionNoLibDBus1
SOURCES += ../qdbusconnection_no_bus/tst_qdbusconnection_no_bus.cpp
