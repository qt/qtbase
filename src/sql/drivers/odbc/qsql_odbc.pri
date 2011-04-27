HEADERS += $$PWD/qsql_odbc.h
SOURCES += $$PWD/qsql_odbc.cpp

unix {
    DEFINES += UNICODE
    !contains(LIBS, .*odbc.*) {
        macx:LIBS += -liodbc
        else:LIBS += $$QT_LFLAGS_ODBC
    }
} else {
    win32-borland:LIBS *= $(BCB)/lib/PSDK/odbc32.lib
    else:LIBS *= -lodbc32
}
