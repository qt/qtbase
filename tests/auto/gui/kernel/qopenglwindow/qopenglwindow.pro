CONFIG += testcase
TARGET = tst_qopenglwindow

QT += core-private gui-private testlib

SOURCES  += tst_qopenglwindow.cpp

win32:CONFIG+=insignificant_test # QTBUG-46452, QTBUG-49630
