# FIXME cmake FindPostgreSQL is more exhaustive than the check we have for libpq-fe.h
# it also checks for catalog/pg_type.h which is a more internal include, we should
# add a way to tell cmake FindPostgreSQL to optionally only look for the libpq-fe.h one
qt_find_package(PostgreSQL) # special case

#####################################################################
## qsqlpsql Plugin:
#####################################################################

add_qt_plugin(qsqlpsql
    TYPE sqldrivers
    CLASS_NAME QPSQLDriverPlugin
    SOURCES
        main.cpp
        qsql_psql.cpp qsql_psql_p.h
    DEFINES
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        PostgreSQL::PostgreSQL
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:psql.pro:<NONE>:
# OTHER_FILES = "psql.json"
# PLUGIN_CLASS_NAME = "QPSQLDriverPlugin"
# _LOADED = "qt_plugin"
