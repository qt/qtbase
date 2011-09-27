load(qttest_p4)
SOURCES  += tst_qsqlquery.cpp

QT = core sql


!wince*:win32:LIBS += -lws2_32


wince*: {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
}
