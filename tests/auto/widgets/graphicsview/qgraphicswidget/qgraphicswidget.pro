CONFIG += testcase
TARGET = tst_qgraphicswidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicswidget.cpp

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test # QTBUG-23616
