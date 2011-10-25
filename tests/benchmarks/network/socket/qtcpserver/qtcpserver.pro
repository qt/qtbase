TEMPLATE = app
TARGET = tst_bench_qtcpserver
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

CONFIG += release

# Input
SOURCES += tst_qtcpserver.cpp
