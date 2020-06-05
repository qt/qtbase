# Generated from qlockfile.pro.

#####################################################################
## tst_qlockfile Test:
#####################################################################

add_qt_test(tst_qlockfile
    SOURCES
        tst_qlockfile.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
        Qt::CorePrivate
)

## Scopes:
#####################################################################

extend_target(tst_qlockfile CONDITION WIN32
    PUBLIC_LIBRARIES
        advapi32
)
add_subdirectory(qlockfiletesthelper)
