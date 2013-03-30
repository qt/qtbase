CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsql
SOURCES  += tst_qsql.cpp

QT += sql sql-private gui testlib core-private

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
}
win32-g++*: LIBS += -lws2_32

