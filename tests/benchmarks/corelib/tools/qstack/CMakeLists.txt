#####################################################################
## tst_bench_qstack Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qstack
    SOURCES
        tst_bench_qstack.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
