CONFIG += testcase
TARGET = tst_qsqlrelationaltablemodel
SOURCES  += tst_qsqlrelationaltablemodel.cpp

QT = core sql testlib core-private sql-private

wince*: {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
} else {
   win32-g++* {
        LIBS += -lws2_32
   } else:win32 {
        LIBS += ws2_32.lib
   }
}
