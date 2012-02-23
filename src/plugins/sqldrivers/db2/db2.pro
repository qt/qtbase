TARGET = qsqldb2

SOURCES = main.cpp
OTHER_FILES += db2.json
include(../../../sql/drivers/db2/qsql_db2.pri)

include(../qsqldriverbase.pri)
