TEMPLATE = app
TARGET = tst_bench_qsslsocket

QT -= gui
QT += network testlib

CONFIG += release

SOURCES += tst_qsslsocket.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
