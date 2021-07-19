# Generated from qvector.pro.

#####################################################################
## tst_bench_qvector Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qvector
    SOURCES
        tst_bench_qvector.cpp
        outofline.cpp
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
