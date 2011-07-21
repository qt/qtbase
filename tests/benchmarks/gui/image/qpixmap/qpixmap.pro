load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

QT += gui-private

TEMPLATE = app
TARGET = tst_bench_qpixmap

SOURCES += tst_qpixmap.cpp
