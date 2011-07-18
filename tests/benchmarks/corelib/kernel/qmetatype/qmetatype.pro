load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

QT = core
TEMPLATE = app
TARGET = tst_qmetatype

SOURCES += tst_qmetatype.cpp

