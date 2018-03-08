CONFIG += testcase
TARGET = tst_qicon

QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc tst_qicon.cpp

TESTDATA += icons/* second_icons/* fallback_icons/* *.png *.svg *.svgz
