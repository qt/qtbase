CONFIG += testcase
TARGET = tst_qdbusconnection_no_bus
QT = core dbus testlib
SOURCES += tst_qdbusconnection_no_bus.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
