# Generated from qsqlquery.pro.

#####################################################################
## tst_bench_qsqlquery Binary:
#####################################################################

add_qt_benchmark(tst_bench_qsqlquery
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
        Qt::Test
)
