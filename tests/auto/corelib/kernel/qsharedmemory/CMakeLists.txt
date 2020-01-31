# Generated from qsharedmemory.pro.


if(QT_FEATURE_sharedmemory)
    #####################################################################
    ## tst_qsharedmemory Test:
    #####################################################################

    add_qt_test(tst_qsharedmemory
        SOURCES
            tst_qsharedmemory.cpp
        LIBRARIES
            Qt::CorePrivate
    )

#### Keys ignored in scope 4:.:.:test.pro:<TRUE>:
    # CONFIG = "testcase"

    ## Scopes:
    #####################################################################

    extend_target(tst_qsharedmemory CONDITION LINUX
    PUBLIC_LIBRARIES
        rt
    )

    if(NOT WINRT)
        add_subdirectory(producerconsumer)
    endif()
endif()
