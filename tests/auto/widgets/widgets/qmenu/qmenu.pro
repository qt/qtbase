CONFIG += testcase
TARGET = tst_qmenu
QT += widgets testlib
SOURCES  += tst_qmenu.cpp

win32:CONFIG += insignificant_test # QTBUG-24325
