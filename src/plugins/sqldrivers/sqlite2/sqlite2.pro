TARGET = qsqlite2

HEADERS += $$PWD/qsql_sqlite2_p.h
SOURCES += $$PWD/qsql_sqlite2.cpp $$PWD/smain.cpp

!contains(LIBS, .*sqlite.*):LIBS += -lsqlite

OTHER_FILES += sqlite2.json

PLUGIN_CLASS_NAME = QSQLite2DriverPlugin
include(../qsqldriverbase.pri)
