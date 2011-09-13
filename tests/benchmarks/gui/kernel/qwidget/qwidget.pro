load(qttest_p4)

QT += widgets
# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_bench_qwidget
SOURCES += tst_qwidget.cpp
