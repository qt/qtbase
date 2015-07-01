CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsql
SOURCES  += tst_qsql.cpp

QT = core-private sql-private testlib

wince {
   DEPLOYMENT_PLUGIN += qsqlite
}
mingw: LIBS += -lws2_32

