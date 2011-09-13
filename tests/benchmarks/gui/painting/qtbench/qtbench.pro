load(qttest_p4)
QT += widgets

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_qtbench

SOURCES += tst_qtbench.cpp

