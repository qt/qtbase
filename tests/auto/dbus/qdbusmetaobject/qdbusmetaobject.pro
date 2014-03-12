CONFIG += testcase
TARGET = tst_qdbusmetaobject
QT = core dbus-private testlib
SOURCES += tst_qdbusmetaobject.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
