# Generated from qsqldriver.pro.

#####################################################################
## tst_qsqldriver Test:
#####################################################################

add_qt_test(tst_qsqldriver
    SOURCES
        tst_qsqldriver.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
)
