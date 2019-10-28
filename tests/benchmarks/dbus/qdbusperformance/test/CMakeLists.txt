# Generated from test.pro.

#####################################################################
## tst_qdbusperformance Binary:
#####################################################################

add_qt_benchmark(tst_qdbusperformance
    SOURCES
        ../serverobject.h
        ../tst_qdbusperformance.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
        Qt::Test
)
