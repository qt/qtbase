# Generated from qtextbrowser.pro.

#####################################################################
## tst_qtextbrowser Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.html)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.md)
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "markdown.really")
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    subdir/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qtextbrowser
    SOURCES
        tst_qtextbrowser.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtextbrowser CONDITION builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
