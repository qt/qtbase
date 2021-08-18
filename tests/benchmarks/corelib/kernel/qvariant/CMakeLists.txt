#####################################################################
## tst_bench_qvariant Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qvariant
    SOURCES
        tst_bench_qvariant.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qvariant CONDITION NOT TARGET Qt::Gui
    PUBLIC_LIBRARIES
        # Remove: gui
)
