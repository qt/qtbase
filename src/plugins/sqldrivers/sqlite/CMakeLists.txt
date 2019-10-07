# Generated from sqlite.pro.

#####################################################################
## qsqlite Plugin:
#####################################################################

add_qt_plugin(qsqlite
    TYPE sqldrivers
    CLASS_NAME QSQLiteDriverPlugin
    SOURCES
        qsql_sqlite.cpp qsql_sqlite_p.h
        smain.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
        SQLite::SQLite3 # special case
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Sql
)

#### Keys ignored in scope 1:.:.:sqlite.pro:<TRUE>:
# OTHER_FILES = "sqlite.json"
# QT_FOR_CONFIG = "sqldrivers-private"

## Scopes:
#####################################################################

# special case begin
# Remove non-system-sqlite code
# extend_target(qsqlite CONDITION QT_FEATURE_system_sqlite
# special case end
