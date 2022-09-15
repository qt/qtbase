# Generated from mimer.pro.

#####################################################################
## MIMERSQLDriverPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMimerSQLDriverPlugin
    OUTPUT_NAME qsqlmimer
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_mimer.cpp qsql_mimer.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        MimerSQL::MimerSQL
        Qt::Core
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:.:mimer.pro:<TRUE>:
# OTHER_FILES = "mimer.json"
