load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qstringbuilder

QMAKE_CXXFLAGS += -g
QMAKE_CFLAGS += -g

QT -= gui

CONFIG += release

SOURCES += main.cpp 
