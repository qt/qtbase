load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qfileinfo
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

QT += core-private

CONFIG += release

# Input
SOURCES += main.cpp
