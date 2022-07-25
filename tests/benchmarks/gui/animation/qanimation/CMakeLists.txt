# Generated from qanimation.pro.

#####################################################################
## tst_bench_qanimation Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qanimation
    SOURCES
        dummyanimation.cpp dummyanimation.h
        dummyobject.cpp dummyobject.h
        main.cpp
        rectanimation.cpp rectanimation.h
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
