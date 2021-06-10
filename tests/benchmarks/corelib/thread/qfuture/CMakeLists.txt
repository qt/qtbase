#####################################################################
## tst_bench_qfuture Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qfuture
    EXCEPTIONS
    SOURCES
        tst_qfuture.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)
