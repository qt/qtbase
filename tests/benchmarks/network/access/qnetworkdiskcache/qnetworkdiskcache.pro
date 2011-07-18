load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qnetworkdiskcache
DEPENDPATH += .
INCLUDEPATH += .

QT += gui # for QDesktopServices
QT += network testlib

CONFIG += release

# Input
SOURCES += tst_qnetworkdiskcache.cpp


