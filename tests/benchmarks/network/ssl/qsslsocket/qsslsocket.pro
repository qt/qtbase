TEMPLATE = app
TARGET = tst_bench_qsslsocket
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

CONFIG += release

# Input
SOURCES += tst_qsslsocket.cpp
