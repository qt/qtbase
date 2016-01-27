TARGET = tst_qsqlquery
CONFIG += testcase

SOURCES  += tst_qsqlquery.cpp

QT = core sql testlib core-private sql-private

win32:!wince: LIBS += -lws2_32

wince {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
}
