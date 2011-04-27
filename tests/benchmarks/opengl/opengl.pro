load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_opengl
DEPENDPATH += .
INCLUDEPATH += .

QT += opengl

# Input
SOURCES += main.cpp

include(../trusted-benchmarks.pri)