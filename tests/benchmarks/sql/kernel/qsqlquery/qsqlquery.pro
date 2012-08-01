TARGET = tst_bench_qsqlquery

SOURCES += main.cpp

QT = core sql testlib
win32: LIBS += -lws2_32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
