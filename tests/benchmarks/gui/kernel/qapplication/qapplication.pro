QT += widgets testlib

TEMPLATE = app
TARGET = tst_bench_qapplication

CONFIG += release

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
