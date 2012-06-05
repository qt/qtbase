CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qicon

QT += testlib
!contains(QT_CONFIG, no-widgets): QT += widgets
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc

wince* {
   QT += xml svg
   DEPLOYMENT_PLUGIN += qsvg
}
TESTDATA += icons/* *.png *.svg *.svgz
