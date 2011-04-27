load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qsslsocket
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network

CONFIG += release

# Input
SOURCES += tst_qsslsocket.cpp
