CONFIG += testcase
TARGET = tst_networkselftest

SOURCES += tst_networkselftest.cpp
QT = core network testlib

win32:CONFIG += insignificant_test # QTBUG-27571
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
