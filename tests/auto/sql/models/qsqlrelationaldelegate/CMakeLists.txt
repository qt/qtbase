# Generated from qsqlrelationaldelegate.pro.

#####################################################################
## tst_qsqlrelationaldelegate Test:
#####################################################################

add_qt_test(tst_qsqlrelationaldelegate
    SOURCES
        tst_qsqlrelationaldelegate.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
        Qt::Widgets
)
