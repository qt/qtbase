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
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
        SQLite::SQLite3
)

#### Keys ignored in scope 1:.:sqlite.pro:<NONE>:
# OTHER_FILES = "sqlite.json"
# PLUGIN_CLASS_NAME = "QSQLiteDriverPlugin"
# QT_FOR_CONFIG = "sqldrivers-private"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

# special case: Remove dead stuff
# extend_target(qsqlite CONDITION (NOT (ON)) AND (CONFIG(release,debug OR release)) ...
