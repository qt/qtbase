TEMPLATE = app
TARGET = tst_bench_opengl

QT += core-private gui-private opengl opengl-private testlib

SOURCES += main.cpp

include(../trusted-benchmarks.pri)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
