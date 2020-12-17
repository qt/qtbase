# Generated from qstandardpaths.pro.

#####################################################################
## tst_qstandardpaths Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qstandardpaths.cpp")

qt_internal_add_test(tst_qstandardpaths
    SOURCES
        tst_qstandardpaths.cpp
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstandardpaths CONDITION boot2qt
    DEFINES
        SKIP_FINDEXECUTABLE
)
