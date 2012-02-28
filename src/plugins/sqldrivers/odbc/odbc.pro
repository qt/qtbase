TARGET = qsqlodbc

SOURCES = main.cpp
OTHER_FILES += odbc.json
include(../../../sql/drivers/odbc/qsql_odbc.pri)

include(../qsqldriverbase.pri)
