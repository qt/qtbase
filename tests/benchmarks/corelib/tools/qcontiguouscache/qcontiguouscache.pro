TARGET = tst_bench_qcontiguouscache

SOURCES += main.cpp

CONFIG += parallel_test

QT = core testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
