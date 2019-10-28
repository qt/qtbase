# Generated from qvector.pro.

#####################################################################
## tst_bench_vector Binary:
#####################################################################

add_qt_benchmark(tst_bench_vector
    SOURCES
        main.cpp
        outofline.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Test
)
