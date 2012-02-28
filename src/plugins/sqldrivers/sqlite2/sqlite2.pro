TARGET = qsqlite2

SOURCES = smain.cpp
OTHER_FILES += sqlite2.json
include(../../../sql/drivers/sqlite2/qsql_sqlite2.pri)

include(../qsqldriverbase.pri)
