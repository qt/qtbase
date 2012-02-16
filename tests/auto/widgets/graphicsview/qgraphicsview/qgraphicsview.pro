CONFIG += testcase
TARGET = tst_qgraphicsview

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsview.cpp tst_qgraphicsview_2.cpp
DEFINES += QT_NO_CAST_TO_ASCII

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG+=insignificant_test

win32:CONFIG += insignificant_test # QTBUG-24296
