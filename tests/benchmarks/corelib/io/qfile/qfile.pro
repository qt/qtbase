TEMPLATE = app
CONFIG += benchmark
QT = core core-private testlib
win32: DEFINES+= _CRT_SECURE_NO_WARNINGS

TARGET = tst_bench_qfile
SOURCES += main.cpp
