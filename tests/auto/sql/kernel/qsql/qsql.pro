CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsql
SOURCES  += tst_qsql.cpp

QT += sql sql-private gui testlib

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
