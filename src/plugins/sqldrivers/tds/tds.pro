TARGET = qsqltds

HEADERS += $$PWD/qsql_tds_p.h
SOURCES += $$PWD/qsql_tds.cpp $$PWD/main.cpp

unix|mingw: {
    LIBS += $$QMAKE_LIBS_TDS
    !contains(LIBS, .*sybdb.*):LIBS += -lsybdb
    QMAKE_CXXFLAGS *= $$QMAKE_CFLAGS_TDS
} else {
    LIBS *= -lNTWDBLIB
}

OTHER_FILES += tds.json

PLUGIN_CLASS_NAME = QTDSDriverPlugin
include(../qsqldriverbase.pri)
