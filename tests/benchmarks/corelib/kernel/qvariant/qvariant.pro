load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qvariant
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += release
#CONFIG += debug


SOURCES += tst_qvariant.cpp
