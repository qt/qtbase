CONFIG += testcase
TARGET = tst_qgraphicswidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicswidget.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
