load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qiodevice
TARGET.EPOCHEAPSIZE = 0x100000 0x2000000
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp
