TARGET = qsqlpsql

HEADERS += $$PWD/qsql_psql_p.h
SOURCES += $$PWD/qsql_psql.cpp $$PWD/main.cpp

unix|mingw {
    LIBS += $$QMAKE_LIBS_PSQL
    !contains(LIBS, .*pq.*):LIBS += -lpq
    QMAKE_CXXFLAGS *= $$QMAKE_CFLAGS_PSQL
} else {
    !contains(LIBS, .*pq.*):LIBS += -llibpq -lws2_32 -ladvapi32
}

OTHER_FILES += psql.json

PLUGIN_CLASS_NAME = QPSQLDriverPlugin
include(../qsqldriverbase.pri)
