load(qttest_p4)
SOURCES  += tst_qsqltablemodel.cpp

QT = core sql

wince*: {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
} else {
   win32:LIBS += -lws2_32
}

