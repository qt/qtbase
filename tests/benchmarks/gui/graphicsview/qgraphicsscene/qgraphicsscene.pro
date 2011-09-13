load(qttest_p4)
QT += widgets

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qgraphicsscene

SOURCES += tst_qgraphicsscene.cpp

