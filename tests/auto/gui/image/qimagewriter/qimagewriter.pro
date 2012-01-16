CONFIG += testcase
TARGET = tst_qimagewriter
QT += widgets testlib
SOURCES += tst_qimagewriter.cpp
MOC_DIR=tmp
!contains(QT_CONFIG, no-tiff):DEFINES += QTEST_HAVE_TIFF
win32-msvc:QMAKE_CXXFLAGS -= -Zm200
win32-msvc:QMAKE_CXXFLAGS += -Zm800

TESTDATA += images/*
