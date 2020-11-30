#####################################################################
## tst_bench_qcollator Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qcollator
    SOURCES
        tst_bench_qcollator.cpp
    PUBLIC_LIBRARIES
        Qt::Test
        Qt::CorePrivate
)
# Need to test feature ICU, so need CorePrivate.
