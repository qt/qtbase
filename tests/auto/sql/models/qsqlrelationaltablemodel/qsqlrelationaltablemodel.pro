load(qttest_p4)
SOURCES  += tst_qsqlrelationaltablemodel.cpp

QT = core sql

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
