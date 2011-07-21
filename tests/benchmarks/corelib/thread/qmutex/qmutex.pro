load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qmutex
QT -= gui
SOURCES += tst_qmutex.cpp

