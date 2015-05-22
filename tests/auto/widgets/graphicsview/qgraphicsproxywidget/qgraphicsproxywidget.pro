CONFIG += testcase
TARGET = tst_qgraphicsproxywidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsproxywidget.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
