load(qttest_p4)
SOURCES  += tst_qsql.cpp

QT += sql sql-private gui widgets

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
}
