CONFIG += testcase
TARGET = tst_baseline_stylesheet
QT += widgets testlib gui-private

SOURCES += tst_baseline_stylesheet.cpp
RESOURCES += icons.qrc

include($$PWD/../shared/qbaselinetest.pri)

TESTDATA += qss/*
