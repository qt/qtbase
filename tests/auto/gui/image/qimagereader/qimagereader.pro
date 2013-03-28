CONFIG += testcase
TARGET = tst_qimagereader
SOURCES += tst_qimagereader.cpp
MOC_DIR=tmp
QT += core-private gui-private network testlib
RESOURCES += qimagereader.qrc

win32-msvc:QMAKE_CXXFLAGS -= -Zm200
win32-msvc:QMAKE_CXXFLAGS += -Zm800
win32-msvc.net:QMAKE_CXXFLAGS -= -Zm300
win32-msvc.net:QMAKE_CXXFLAGS += -Zm1100

TESTDATA += images/* baseline/*
