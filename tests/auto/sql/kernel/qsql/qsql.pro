CONFIG += testcase
TARGET = tst_qsql
SOURCES  += tst_qsql.cpp

QT += sql sql-private gui widgets testlib

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
}
