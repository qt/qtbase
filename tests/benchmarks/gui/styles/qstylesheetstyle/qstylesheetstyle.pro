load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
QT += widgets
TARGET = tst_bench_qstylesheetstyle
DEPENDPATH += .
INCLUDEPATH += .


CONFIG += release

# Input
SOURCES += main.cpp
