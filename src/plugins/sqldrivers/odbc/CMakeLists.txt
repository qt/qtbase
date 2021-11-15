# Generated from odbc.pro.

#####################################################################
## QODBCDriverPlugin Plugin:
#####################################################################

qt_find_package(ODBC) # special case

qt_internal_add_plugin(QODBCDriverPlugin
    OUTPUT_NAME qsqlodbc
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_odbc.cpp qsql_odbc_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        ODBC::ODBC
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:.:odbc.pro:<TRUE>:
# OTHER_FILES = "odbc.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QODBCDriverPlugin CONDITION UNIX
    DEFINES
        UNICODE
)
