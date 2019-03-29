find_package(ODBC) # special case

#####################################################################
## qsqlodbc Plugin:
#####################################################################

add_qt_plugin(qsqlodbc
    TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_odbc.cpp qsql_odbc_p.h
    DEFINES
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        ODBC::ODBC
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:odbc.pro:<NONE>:
# OTHER_FILES = "odbc.json"
# PLUGIN_CLASS_NAME = "QODBCDriverPlugin"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

extend_target(qsqlodbc CONDITION UNIX
    DEFINES
        UNICODE
)
