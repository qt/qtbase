CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qicon

QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc

TESTDATA += icons/* *.png *.svg *.svgz
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
