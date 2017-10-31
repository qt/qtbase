CONFIG += testcase
TARGET = tst_qgraphicsproxywidget

QT += widgets widgets-private testlib
QT += core-private gui-private

DEFINES += QTEST_QPA_MOUSE_HANDLING
SOURCES  += tst_qgraphicsproxywidget.cpp

