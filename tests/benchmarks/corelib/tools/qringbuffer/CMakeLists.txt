#####################################################################
## tst_bench_qringbuffer Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qringbuffer
    SOURCES
        tst_bench_qringbuffer.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
