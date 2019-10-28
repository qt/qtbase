# Generated from qanimation.pro.

#####################################################################
## tst_bench_qanimation Binary:
#####################################################################

add_qt_benchmark(tst_bench_qanimation
    SOURCES
        dummyanimation.cpp dummyanimation.h
        dummyobject.cpp dummyobject.h
        main.cpp
        rectanimation.cpp rectanimation.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
