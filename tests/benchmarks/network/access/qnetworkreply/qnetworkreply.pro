TEMPLATE = app
TARGET = tst_bench_qnetworkreply
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

CONFIG += release

# Input
SOURCES += tst_qnetworkreply.cpp
