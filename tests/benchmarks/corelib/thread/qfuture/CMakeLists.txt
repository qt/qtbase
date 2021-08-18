#####################################################################
## tst_bench_qfuture Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qfuture
    EXCEPTIONS
    SOURCES
        tst_bench_qfuture.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)
