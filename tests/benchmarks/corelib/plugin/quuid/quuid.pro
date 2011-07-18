load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_quuid

SOURCES += tst_quuid.cpp
QT -= gui
