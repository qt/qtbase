#####################################################################
## tst_bench_qlocalsocket Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qlocalsocket.pro:<TRUE>:
# TEMPLATE = "app"
