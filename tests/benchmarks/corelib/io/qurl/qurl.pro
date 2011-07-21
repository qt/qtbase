load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_qurl
QT -= gui
win32: DEFINES+= _CRT_SECURE_NO_WARNINGS

SOURCES += main.cpp
