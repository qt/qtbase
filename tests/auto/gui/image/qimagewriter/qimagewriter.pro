CONFIG += testcase
TARGET = tst_qimagewriter
QT += testlib
SOURCES += tst_qimagewriter.cpp
MOC_DIR=tmp
win32-msvc:QMAKE_CXXFLAGS -= -Zm200
win32-msvc:QMAKE_CXXFLAGS += -Zm800

TESTDATA += images/*
