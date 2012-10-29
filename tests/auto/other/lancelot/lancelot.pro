CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_lancelot
QT += xml widgets testlib

SOURCES += tst_lancelot.cpp \
           paintcommands.cpp
HEADERS += paintcommands.h
RESOURCES += images.qrc

include($$PWD/../../../baselineserver/shared/qbaselinetest.pri)

TESTDATA += scripts/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
