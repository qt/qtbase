CONFIG += testcase
TARGET = tst_lancelot
QT += testlib gui-private

SOURCES += tst_lancelot.cpp \
           paintcommands.cpp
HEADERS += paintcommands.h
RESOURCES += images.qrc

include($$PWD/../../../baselineserver/shared/qbaselinetest.pri)

TESTDATA += scripts/*
