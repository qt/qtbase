load(qttest_p4)

QT += widgets
QT += gui-private widgets-private
# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_QText

SOURCES += main.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
