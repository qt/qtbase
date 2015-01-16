TARGET = qsqlite

SOURCES = smain.cpp
OTHER_FILES += sqlite.json
include(../../../sql/drivers/sqlite/qsql_sqlite.pri)

PLUGIN_CLASS_NAME = QSQLiteDriverPlugin
include(../qsqldriverbase.pri)
