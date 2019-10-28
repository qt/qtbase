# Generated from qsqlrecord.pro.

#####################################################################
## tst_bench_qsqlrecord Binary:
#####################################################################

add_qt_benchmark(tst_bench_qsqlrecord
    SOURCES
        tst_qsqlrecord.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Qt::Sql
        Qt::Test
)
