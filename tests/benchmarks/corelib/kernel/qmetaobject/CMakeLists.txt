#####################################################################
## tst_bench_qmetaobject Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qmetaobject
    SOURCES
        tst_bench_qmetaobject.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
