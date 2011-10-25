TEMPLATE = app
TARGET = tst_bench_qfile_vs_qnetworkaccessmanager
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

CONFIG += release

# Input
SOURCES += main.cpp
