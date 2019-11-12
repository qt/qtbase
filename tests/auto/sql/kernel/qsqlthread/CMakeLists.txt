# Generated from qsqlthread.pro.

#####################################################################
## tst_qsqlthread Test:
#####################################################################

add_qt_test(tst_qsqlthread
    SOURCES
        tst_qsqlthread.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
)
