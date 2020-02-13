# Generated from qsqlrecord.pro.

#####################################################################
## tst_bench_qsqlrecord Binary:
#####################################################################

add_qt_benchmark(tst_bench_qsqlrecord
    SOURCES
        tst_qsqlrecord.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
        Qt::Test
)
