#####################################################################
## tst_bench_qobject Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qobject
    SOURCES
        tst_bench_qobject.cpp
        object.cpp object.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
