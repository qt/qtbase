# Generated from qsharedmemory.pro.

if(QT_FEATURE_sharedmemory) # special case
    #####################################################################
    ## tst_qsharedmemory Test:
    #####################################################################

    add_qt_test(tst_qsharedmemory
        SOURCES
            tst_qsharedmemory.cpp
        PUBLIC_LIBRARIES
            Qt::CorePrivate
    )

    ## Scopes:
    #####################################################################

    extend_target(tst_qsharedmemory CONDITION LINUX
    PUBLIC_LIBRARIES
        rt
    )
# if(QT_FEATURE_sharedmemory) # special case
    add_subdirectory(producerconsumer)
endif()
