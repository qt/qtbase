load(qttest_p4)
SOURCES  += tst_qsqlrelationaltablemodel.cpp

QT += sql

wince*: {
   plugFiles.files = ../../../plugins/sqldrivers
   plugFiles.path    = .
   DEPLOYMENT += plugFiles
   LIBS += -lws2
}else:symbian {
    qt_not_deployed {
        contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
            sqlite.path = /sys/bin
            sqlite.files = sqlite3.dll
            DEPLOYMENT += sqlite
        }
    }
} else {
   win32-g++* {
        LIBS += -lws2_32
   } else:win32 {
        LIBS += ws2_32.lib
   }
}
