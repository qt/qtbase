CONFIG += testcase
TARGET = tst_qgraphicswidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicswidget.cpp


mac*:CONFIG+=insignificant_test
qpa:contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-20778 unstable on qpa, xcb
