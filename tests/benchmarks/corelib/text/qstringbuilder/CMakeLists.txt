#####################################################################
## tst_bench_qstringbuilder Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qstringbuilder
    SOURCES
        tst_bench_qstringbuilder.cpp
    LIBRARIES
        Qt::Test
    COMPILE_OPTIONS
        -g
)
