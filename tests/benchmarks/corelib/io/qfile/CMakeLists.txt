# Generated from qfile.pro.

#####################################################################
## tst_bench_qfile Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qfile
    SOURCES
        tst_bench_qfile.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qfile.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qfile CONDITION WIN32
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
