HEADERS += $$PWD/qsql_sqlite2.h
SOURCES += $$PWD/qsql_sqlite2.cpp

!contains(LIBS, .*sqlite.*):LIBS += -lsqlite
