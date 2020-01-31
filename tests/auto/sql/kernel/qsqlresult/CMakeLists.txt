# Generated from qsqlresult.pro.

#####################################################################
## tst_qsqlresult Test:
#####################################################################

add_qt_test(tst_qsqlresult
    SOURCES
        testsqldriver.h
        tst_qsqlresult.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
)
