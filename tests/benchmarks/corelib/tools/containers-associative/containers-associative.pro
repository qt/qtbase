load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_containers-associative
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp
QT -= gui
