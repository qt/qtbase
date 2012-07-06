CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qicon

QT += testlib
!contains(QT_CONFIG, no-widgets): QT += widgets
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc

TESTDATA += icons/* *.png *.svg *.svgz
