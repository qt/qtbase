load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

SOURCES += ../tst_qdbusperformance.cpp
HEADERS += ../serverobject.h
TARGET = ../tst_qdbusperformance

QT = core
CONFIG += qdbus
