load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qgraphicswidget
TEMPLATE = app
# Input
SOURCES += tst_qgraphicswidget.cpp
