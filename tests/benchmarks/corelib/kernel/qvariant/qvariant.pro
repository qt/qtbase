TARGET = tst_bench_qvariant
QT += testlib
!qtHaveModule(gui): QT -= gui

CONFIG += release
#CONFIG += debug


SOURCES += tst_qvariant.cpp
