# Generated from qtextbrowser.pro.

#####################################################################
## tst_qtextbrowser Test:
#####################################################################

add_qt_test(tst_qtextbrowser
    SOURCES
        tst_qtextbrowser.cpp
    PUBLIC_LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qtextbrowser.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "*.html" "*.md" "subdir/*"

## Scopes:
#####################################################################

extend_target(tst_qtextbrowser CONDITION builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
