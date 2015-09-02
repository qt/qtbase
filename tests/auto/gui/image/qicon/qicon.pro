CONFIG += testcase
TARGET = tst_qicon

QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc

TESTDATA += icons/* second_icons/* *.png *.svg *.svgz
