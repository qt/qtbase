load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_bench_qwidget
SOURCES += tst_qwidget.cpp
