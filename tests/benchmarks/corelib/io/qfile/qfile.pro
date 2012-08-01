TEMPLATE = app
TARGET = tst_bench_qfile
QT = core core-private testlib
win32: DEFINES+= _CRT_SECURE_NO_WARNINGS

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
