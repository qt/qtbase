if(QT_FEATURE_sharedmemory)
    #####################################################################
    ## tst_qsharedmemory Test:
    #####################################################################

    qt_internal_add_test(tst_qsharedmemory
        SOURCES
            tst_qsharedmemory.cpp
        PUBLIC_LIBRARIES
            Qt::CorePrivate
    )

    ## Scopes:
    #####################################################################

    qt_internal_extend_target(tst_qsharedmemory CONDITION LINUX
        PUBLIC_LIBRARIES
            rt
    )
    add_subdirectory(producerconsumer)
    if(QT_FEATURE_process)
        add_dependencies(tst_qsharedmemory producerconsumer_helper)
    endif()
endif()
