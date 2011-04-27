load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qtcpserver
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network

CONFIG += release

# Input
SOURCES += tst_qtcpserver.cpp
