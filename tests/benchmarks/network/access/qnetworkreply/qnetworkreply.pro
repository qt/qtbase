load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qnetworkreply
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network

CONFIG += release

# Input
SOURCES += tst_qnetworkreply.cpp
