CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qopenglwidget
QT += gui-private core-private testlib widgets

SOURCES   += tst_qopenglwidget.cpp

win32-msvc2010:contains(QT_CONFIG, angle):CONFIG += insignificant_test # QTBUG-31611
