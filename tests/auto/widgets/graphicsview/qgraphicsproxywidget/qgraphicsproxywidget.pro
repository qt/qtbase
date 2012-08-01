CONFIG += testcase
TARGET = tst_qgraphicsproxywidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsproxywidget.cpp

contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-25294
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
