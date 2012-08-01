TEMPLATE = app
TARGET = tst_bench_qnetworkdiskcache

QT += gui # for QDesktopServices
QT += network testlib

CONFIG += release

SOURCES += tst_qnetworkdiskcache.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
