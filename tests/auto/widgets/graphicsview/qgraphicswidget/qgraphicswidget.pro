CONFIG += testcase
TARGET = tst_qgraphicswidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicswidget.cpp

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = lucid ]"):DEFINES+=UBUNTU_LUCID # QTBUG-20778
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
