CONFIG += benchmark
QT += testlib
!qtHaveModule(gui): QT -= gui

TARGET = tst_bench_qvariant
SOURCES += tst_qvariant.cpp
