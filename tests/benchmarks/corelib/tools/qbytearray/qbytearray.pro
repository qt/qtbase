TEMPLATE = app
TARGET = tst_bench_qbytearray

QT = core testlib
CONFIG += release

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
