TARGET = tst_bench_qsqlquery

SOURCES += main.cpp

QT = core sql testlib
win32: LIBS += -lWs2_32
