# Generated from mysql.pro.

#####################################################################
## qsqlmysql Plugin:
#####################################################################

add_qt_plugin(qsqlmysql
    TYPE sqldrivers
    CLASS_NAME QMYSQLDriverPlugin
    SOURCES
        main.cpp
        qsql_mysql.cpp qsql_mysql_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        MySQL::MySQL
        Qt::Core
        Qt::Sql
)

#### Keys ignored in scope 1:.:.:mysql.pro:<TRUE>:
# OTHER_FILES = "mysql.json"
# PLUGIN_CLASS_NAME = "QMYSQLDriverPlugin"
