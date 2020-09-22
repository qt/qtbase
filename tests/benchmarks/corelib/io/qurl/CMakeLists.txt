# Generated from qurl.pro.

#####################################################################
## tst_qurl Binary:
#####################################################################

qt_internal_add_benchmark(tst_qurl
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qurl.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qurl CONDITION WIN32
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
