# Generated from qlockfile.pro.

#####################################################################
## tst_qlockfile Test:
#####################################################################

add_qt_test(tst_qlockfile
    SOURCES
        tst_qlockfile.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

## Scopes:
#####################################################################

extend_target(tst_qlockfile CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        advapi32
)
add_subdirectory(qlockfiletesthelper)
