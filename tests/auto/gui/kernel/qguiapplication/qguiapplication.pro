CONFIG += testcase
TARGET = tst_qguiapplication
QT += core gui testlib
SOURCES = tst_qguiapplication.cpp

win32:CONFIG += insignificant_test # QTBUG-24186
