TEMPLATE = app
TARGET = tst_bench_qhostinfo

QT -= gui
QT += core-private network network-private testlib

CONFIG += release

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
