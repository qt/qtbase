load(qttest_p4)

QT += widgets
QT += gui-private widgets-private
# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qpainter

SOURCES += tst_qpainter.cpp
