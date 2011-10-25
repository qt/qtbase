CONFIG += testcase
TARGET = tst_qscrollbar
QT += widgets testlib
SOURCES += tst_qscrollbar.cpp

mac*:CONFIG+=insignificant_test
