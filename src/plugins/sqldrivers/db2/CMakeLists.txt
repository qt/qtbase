# Generated from db2.pro.

#####################################################################
## qsqldb2 Plugin:
#####################################################################

add_qt_plugin(qsqldb2
    TYPE sqldrivers
    CLASS_NAME QDB2DriverPlugin
    SOURCES
        main.cpp
        qsql_db2.cpp qsql_db2_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        DB2::DB2
        Qt::Core
        Qt::Sql
)

#### Keys ignored in scope 1:.:.:db2.pro:<TRUE>:
# OTHER_FILES = "db2.json"
# PLUGIN_CLASS_NAME = "QDB2DriverPlugin"

## Scopes:
#####################################################################

extend_target(qsqldb2 CONDITION (TEST_architecture_arch STREQUAL "x86_64")
    DEFINES
        ODBC64
)
