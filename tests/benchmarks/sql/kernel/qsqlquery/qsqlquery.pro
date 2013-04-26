TARGET = tst_bench_qsqlquery

SOURCES += main.cpp

QT = core sql testlib core-private sql-private
LIBS += $$QMAKE_LIBS_NETWORK
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
