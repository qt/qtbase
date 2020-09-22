# Generated from qvector.pro.

#####################################################################
## tst_bench_vector Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_vector
    SOURCES
        main.cpp
        outofline.cpp
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
