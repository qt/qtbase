CONFIG += testcase
TARGET = tst_qimagewriter
QT += testlib
SOURCES += tst_qimagewriter.cpp
MOC_DIR=tmp
android: RESOURCES+= qimagewriter.qrc
TESTDATA += images/*
