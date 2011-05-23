load(qttest_p4)
QT += widgets
TEMPLATE = app
TARGET = tst_bench_qapplication
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += release

# Input
SOURCES += main.cpp
