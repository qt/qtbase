CONFIG += testcase
TARGET = tst_qimagewriter
QT += testlib
SOURCES += tst_qimagewriter.cpp
MOC_DIR=tmp
android:!android-no-sdk:RESOURCES+= qimagewriter.qrc
TESTDATA += images/*
