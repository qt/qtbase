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

symbian {
    qt_not_deployed {
        contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
            sqlite.path = /sys/bin
            sqlite.files = sqlite3.dll
            DEPLOYMENT += sqlite
        }
    }
}
