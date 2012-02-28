TARGET = qsqlibase

SOURCES = main.cpp
OTHER_FILES += ibase.json
include(../../../sql/drivers/ibase/qsql_ibase.pri)

include(../qsqldriverbase.pri)
