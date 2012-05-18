CONFIG += testcase
TARGET = tst_qlistwidget
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qlistwidget.cpp

contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21098, fails unstably
