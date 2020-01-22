# Generated from mysql.pro.

#####################################################################
## QMYSQLDriverPlugin Plugin:
#####################################################################

qt_add_plugin(QMYSQLDriverPlugin
    OUTPUT_NAME qsqlmysql
    TYPE sqldrivers
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
