# Generated from qthread.pro.

#####################################################################
## tst_qthread Test:
#####################################################################

qt_find_package(Threads PROVIDED_TARGETS Threads::Threads) # special case

add_qt_test(tst_qthread
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qthread.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
    LIBRARIES # special case
        Threads::Threads # special case
)

## Scopes:
#####################################################################
