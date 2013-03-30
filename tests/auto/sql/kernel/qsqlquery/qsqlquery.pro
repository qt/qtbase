TARGET = tst_qsqlquery
CONFIG += testcase

SOURCES  += tst_qsqlquery.cpp

QT = core sql testlib core-private sql-private

!wince*:win32:LIBS += -lws2_32

wince*: {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
}
