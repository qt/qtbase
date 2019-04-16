CONFIG += testcase
QT += testlib gui-private core-private

TARGET = tst_qcolorspace
SOURCES  += tst_qcolorspace.cpp

RESOURCES += $$files(resources/*)

TESTDATA += resources/*
