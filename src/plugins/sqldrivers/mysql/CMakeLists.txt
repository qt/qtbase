# Generated from mysql.pro.

#####################################################################
## QMYSQLDriverPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMYSQLDriverPlugin
    OUTPUT_NAME qsqlmysql
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_mysql.cpp qsql_mysql_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        MySQL::MySQL
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:.:mysql.pro:<TRUE>:
# OTHER_FILES = "mysql.json"

qt_internal_force_macos_intel_arch(QMYSQLDriverPlugin)
