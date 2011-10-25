TEMPLATE = app
TARGET = tst_bench_qfile
QT = core testlib
win32: DEFINES+= _CRT_SECURE_NO_WARNINGS

SOURCES += main.cpp
