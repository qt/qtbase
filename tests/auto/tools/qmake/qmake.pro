CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qmake
HEADERS += testcompiler.h
SOURCES += tst_qmake.cpp testcompiler.cpp
QT = core testlib

cross_compile: DEFINES += QMAKE_CROSS_COMPILED

TESTDATA += testdata/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
