CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsqltablemodel
SOURCES  += tst_qsqltablemodel.cpp

QT = core core-private sql sql-private testlib

wince*: {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
} else {
   win32:LIBS += -lws2_32
}

