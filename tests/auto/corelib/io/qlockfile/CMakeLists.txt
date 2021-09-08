#####################################################################
## tst_qlockfile Test:
#####################################################################

qt_internal_add_test(tst_qlockfile
    SOURCES
        tst_qlockfile.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qlockfile CONDITION WIN32
    PUBLIC_LIBRARIES
        advapi32
)
add_subdirectory(qlockfiletesthelper)

if(QT_FEATURE_process AND NOT ANDROID)
    add_dependencies(tst_qlockfile qlockfile_test_helper)
endif()
