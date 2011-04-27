load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qfile
QT -= gui
win32: DEFINES+= _CRT_SECURE_NO_WARNINGS

SOURCES += main.cpp
