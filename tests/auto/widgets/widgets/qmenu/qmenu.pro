CONFIG += testcase
TARGET = tst_qmenu
QT += widgets testlib
SOURCES  += tst_qmenu.cpp

contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21100, unstably fails
