CONFIG += testcase
TARGET = tst_qicoimageformat
SOURCES+= tst_qicoimageformat.cpp
QT += testlib

TESTDATA += icons/*
android:RESOURCES+=qicoimageformat.qrc
