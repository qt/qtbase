load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qstringbuilder

QMAKE_CXXFLAGS += -g
QMAKE_CFLAGS += -g

QT -= gui

CONFIG += release

SOURCES += main.cpp 
