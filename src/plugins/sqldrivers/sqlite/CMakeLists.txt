# Generated from sqlite.pro.

#####################################################################
## QSQLiteDriverPlugin Plugin:
#####################################################################

qt_add_plugin(QSQLiteDriverPlugin
    OUTPUT_NAME qsqlite
    TYPE sqldrivers
    SOURCES
        qsql_sqlite.cpp qsql_sqlite_p.h
        smain.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES # special case
        SQLite::SQLite3 # special case
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:.:sqlite.pro:<TRUE>:
# OTHER_FILES = "sqlite.json"
# QT_FOR_CONFIG = "sqldrivers-private"

## Scopes:
#####################################################################

# special case begin
# Remove non-system-sqlite code
# qt_extend_target(qsqlite CONDITION QT_FEATURE_system_sqlite
# special case end
