# Generated from qnetworkreply.pro.

#####################################################################
## tst_bench_qnetworkreply Binary:
#####################################################################

add_qt_benchmark(tst_bench_qnetworkreply
    SOURCES
        tst_qnetworkreply.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qnetworkreply.pro:<TRUE>:
# TEMPLATE = "app"
