load(qttest_p4)
QT += widgets

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qobject
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += object.h
SOURCES += main.cpp object.cpp
