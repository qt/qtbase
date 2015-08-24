CONFIG += testcase
TARGET = tst_networkselftest

SOURCES += tst_networkselftest.cpp
QT = core core-private network testlib

win32:CONFIG += insignificant_test # QTBUG-27571
