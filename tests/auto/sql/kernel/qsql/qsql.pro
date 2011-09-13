load(qttest_p4)
SOURCES  += tst_qsql.cpp

QT += sql sql-private gui widgets

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
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
