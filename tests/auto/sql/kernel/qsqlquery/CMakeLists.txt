# Generated from qsqlquery.pro.

#####################################################################
## tst_qsqlquery Test:
#####################################################################

add_qt_test(tst_qsqlquery
    SOURCES
        tst_qsqlquery.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
)
