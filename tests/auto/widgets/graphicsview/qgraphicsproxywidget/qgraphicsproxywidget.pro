CONFIG += testcase
TARGET = tst_qgraphicsproxywidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsproxywidget.cpp

contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-25294

win32:CONFIG += insignificant_test # QTBUG-24294
