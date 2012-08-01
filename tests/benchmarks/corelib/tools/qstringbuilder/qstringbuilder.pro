TEMPLATE = app
TARGET = tst_bench_qstringbuilder

QMAKE_CXXFLAGS += -g
QMAKE_CFLAGS += -g

QT = core testlib

CONFIG += release

SOURCES += main.cpp 
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
