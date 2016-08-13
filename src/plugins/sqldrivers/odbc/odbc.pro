TARGET = qsqlodbc

HEADERS += $$PWD/qsql_odbc_p.h
SOURCES += $$PWD/qsql_odbc.cpp $$PWD/main.cpp

QMAKE_USE += odbc
unix: DEFINES += UNICODE

OTHER_FILES += odbc.json

PLUGIN_CLASS_NAME = QODBCDriverPlugin
include(../qsqldriverbase.pri)
