TEMPLATE = app
TARGET = tst_bench_opengl
DEPENDPATH += .
INCLUDEPATH += .

QT += core-private gui-private opengl opengl-private testlib

# Input
SOURCES += main.cpp

include(../trusted-benchmarks.pri)
