load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qrect
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp
