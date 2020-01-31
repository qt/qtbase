# Generated from qfile.pro.

#####################################################################
## tst_bench_qfile Binary:
#####################################################################

add_qt_benchmark(tst_bench_qfile
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qfile.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(tst_bench_qfile CONDITION WIN32
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
