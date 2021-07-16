#####################################################################
## tst_qsharedpointer Test:
#####################################################################

qt_internal_add_test(tst_qsharedpointer
    SOURCES
        forwarddeclared.cpp
        nontracked.cpp
        wrapper.cpp
        tst_qsharedpointer.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
