TARGET = qsqlite

HEADERS += $$PWD/qsql_sqlite_p.h
SOURCES += $$PWD/qsql_sqlite.cpp $$PWD/smain.cpp

!system-sqlite {
    include($$PWD/../../../3rdparty/sqlite.pri)
} else {
    QMAKE_USE += sqlite
}

OTHER_FILES += sqlite.json

PLUGIN_CLASS_NAME = QSQLiteDriverPlugin
include(../qsqldriverbase.pri)
