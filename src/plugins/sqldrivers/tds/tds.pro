TARGET = qsqltds

SOURCES = main.cpp
OTHER_FILES += tds.json
include(../../../sql/drivers/tds/qsql_tds.pri)

include(../qsqldriverbase.pri)
