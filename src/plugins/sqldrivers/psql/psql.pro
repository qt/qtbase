TARGET = qsqlpsql

SOURCES = main.cpp
OTHER_FILES += psql.json
include(../../../sql/drivers/psql/qsql_psql.pri)

include(../qsqldriverbase.pri)
