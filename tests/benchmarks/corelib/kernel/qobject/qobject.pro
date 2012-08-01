QT += widgets testlib

TEMPLATE = app
TARGET = tst_bench_qobject

HEADERS += object.h
SOURCES += main.cpp object.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
