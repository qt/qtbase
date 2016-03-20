TARGET = qsqlite

HEADERS += $$PWD/qsql_sqlite_p.h
SOURCES += $$PWD/qsql_sqlite.cpp $$PWD/smain.cpp

!system-sqlite:!contains(LIBS, .*sqlite3.*) {
    include($$PWD/../../../3rdparty/sqlite.pri)
} else {
    LIBS += $$QMAKE_LIBS_SQLITE
    QMAKE_CXXFLAGS *= $$QMAKE_CFLAGS_SQLITE
}

OTHER_FILES += sqlite.json

PLUGIN_CLASS_NAME = QSQLiteDriverPlugin
include(../qsqldriverbase.pri)
