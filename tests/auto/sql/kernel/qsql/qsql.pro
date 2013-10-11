CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsql
SOURCES  += tst_qsql.cpp

QT = core-private sql-private testlib

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
}
win32-g++*: LIBS += -lws2_32

