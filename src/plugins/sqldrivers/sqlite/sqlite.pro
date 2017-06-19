TARGET = qsqlite

HEADERS += $$PWD/qsql_sqlite_p.h
SOURCES += $$PWD/qsql_sqlite.cpp $$PWD/smain.cpp

include($$OUT_PWD/../qtsqldrivers-config.pri)
QT_FOR_CONFIG += sqldrivers-private

qtConfig(system-sqlite) {
    QMAKE_USE += sqlite
} else {
    include($$PWD/../../../3rdparty/sqlite.pri)
}

OTHER_FILES += sqlite.json

PLUGIN_CLASS_NAME = QSQLiteDriverPlugin
include(../qsqldriverbase.pri)
