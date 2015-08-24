TEMPLATE = app
TARGET = tst_bench_qtcpserver

QT -= gui
QT += network testlib

CONFIG += release

SOURCES += tst_qtcpserver.cpp
