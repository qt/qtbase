TEMPLATE = app
TARGET = tst_bench_qfileinfo
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

QT += core-private testlib

CONFIG += release

# Input
SOURCES += main.cpp
