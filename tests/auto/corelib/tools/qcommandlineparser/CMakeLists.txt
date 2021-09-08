#####################################################################
## tst_qcommandlineparser Test:
#####################################################################

qt_internal_add_test(tst_qcommandlineparser
    SOURCES
        tst_qcommandlineparser.cpp
)
add_subdirectory(testhelper)
if(QT_FEATURE_process AND NOT ANDROID)
    add_dependencies(tst_qcommandlineparser qcommandlineparser_test_helper)
endif()
