#####################################################################
## tst_bench_qreadwritelock Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qreadwritelock
    SOURCES
        tst_bench_qreadwritelock.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
