CONFIG += testcase
TARGET = tst_qdbusconnection
QT = core dbus testlib
SOURCES += tst_qdbusconnection.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
