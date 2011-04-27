HEADERS += $$PWD/qsql_tds.h
SOURCES += $$PWD/qsql_tds.cpp

unix|win32-g++*: {
    LIBS *= $$QT_LFLAGS_TDS
    !contains(LIBS, .*sybdb.*):LIBS += -lsybdb
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_TDS
} else:win32-borland {
    LIBS *= $(BCB)/lib/PSDK/NTWDBLIB.LIB
} else {
    LIBS *= -lNTWDBLIB
}
