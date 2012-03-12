CONFIG += testcase
TARGET = tst_qicon

QT += widgets testlib
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc

wince* {
   QT += xml svg
   DEPLOYMENT_PLUGIN += qsvg
}
TESTDATA += icons/* *.png *.svg *.svgz
