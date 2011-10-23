TARGET = tst_bench_qsqlquery

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

SOURCES += main.cpp

QT = core sql testlib
