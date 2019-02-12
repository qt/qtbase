# Generated from sqlite.pro.

find_package(SQLite3)

#####################################################################
## qsqlite Plugin:
#####################################################################

add_qt_plugin(qsqlite
    TYPE sqldrivers
    SOURCES
        qsql_sqlite.cpp qsql_sqlite_p.h
        smain.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
        SQLite::SQLite3
    # OTHER_FILES = "sqlite.json"
    # PLUGIN_CLASS_NAME = "QSQLiteDriverPlugin"
    # QT_FOR_CONFIG = "sqldrivers-private"
    # _LOADED = "qt_plugin"
)
