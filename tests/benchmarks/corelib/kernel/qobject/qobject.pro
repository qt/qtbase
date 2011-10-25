QT += widgets testlib

TEMPLATE = app
TARGET = tst_bench_qobject
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += object.h
SOURCES += main.cpp object.cpp
