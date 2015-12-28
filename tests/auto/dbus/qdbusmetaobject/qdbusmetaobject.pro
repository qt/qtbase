CONFIG += testcase parallel_test
TARGET = tst_qdbusmetaobject
QT = core dbus-private testlib
SOURCES += tst_qdbusmetaobject.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
