#####################################################################
## tst_bench_qhash Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qhash
    SOURCES
        tst_bench_qhash.cpp
        outofline.cpp
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Test
)
