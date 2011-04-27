load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qvariant
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += release
#CONFIG += debug


SOURCES += tst_qvariant.cpp
