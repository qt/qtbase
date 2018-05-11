TEMPLATE = app
TARGET = tst_bench_lancebench

QT += testlib gui-private


SOURCES += tst_lancebench.cpp

SOURCES += ../../../../auto/other/lancelot/paintcommands.cpp
HEADERS += ../../../../auto/other/lancelot/paintcommands.h
RESOURCES += ../../../../auto/other/lancelot/images.qrc

TESTDATA += ../../../../auto/other/lancelot/scripts/*
