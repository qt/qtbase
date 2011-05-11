load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qfileinfo
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

QT += core-private

CONFIG += release

# Input
SOURCES += main.cpp
