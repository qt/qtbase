#####################################################################
## tst_bench_qmutex Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qmutex
    SOURCES
        tst_bench_qmutex.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
