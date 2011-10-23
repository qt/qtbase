load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_qlist
QT = core

SOURCES += main.cpp
