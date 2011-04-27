TARGET = qsqlite

SOURCES = smain.cpp
include(../../../sql/drivers/sqlite/qsql_sqlite.pri)

wince*: DEFINES += HAVE_LOCALTIME_S=0

include(../qsqldriverbase.pri)
