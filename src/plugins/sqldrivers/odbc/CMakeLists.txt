# Generated from odbc.pro.

find_package(ODBC)

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
        Qt::CorePrivate
        Qt::SqlPrivate
        ODBC::ODBC
    # OTHER_FILES = "odbc.json"
    # PLUGIN_CLASS_NAME = "QODBCDriverPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

extend_target(qsqlodbc CONDITION UNIX
    DEFINES
        UNICODE
)
