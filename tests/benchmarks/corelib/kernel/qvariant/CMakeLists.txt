# Generated from qvariant.pro.

#####################################################################
## tst_bench_qvariant Binary:
#####################################################################

add_qt_benchmark(tst_bench_qvariant
    SOURCES
        tst_qvariant.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

## Scopes:
#####################################################################

extend_target(tst_bench_qvariant CONDITION NOT TARGET Qt::Gui
    PUBLIC_LIBRARIES
        # Remove: gui
)
