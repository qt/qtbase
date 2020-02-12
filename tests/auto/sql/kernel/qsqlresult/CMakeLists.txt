# Generated from qsqlresult.pro.

#####################################################################
## tst_qsqlresult Test:
#####################################################################

add_qt_test(tst_qsqlresult
    SOURCES
        testsqldriver.h
        tst_qsqlresult.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
