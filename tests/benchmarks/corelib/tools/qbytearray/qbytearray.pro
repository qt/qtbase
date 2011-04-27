load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qbytearray
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp
