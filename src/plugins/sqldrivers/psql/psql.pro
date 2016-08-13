TARGET = qsqlpsql

HEADERS += $$PWD/qsql_psql_p.h
SOURCES += $$PWD/qsql_psql.cpp $$PWD/main.cpp

QMAKE_USE += psql

OTHER_FILES += psql.json

PLUGIN_CLASS_NAME = QPSQLDriverPlugin
include(../qsqldriverbase.pri)
