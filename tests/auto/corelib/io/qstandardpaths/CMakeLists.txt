# Generated from qstandardpaths.pro.

#####################################################################
## tst_qstandardpaths Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qstandardpaths.cpp")
list(APPEND test_data "qstandardpaths.pro")

qt_add_test(tst_qstandardpaths
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qstandardpaths.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_extend_target(tst_qstandardpaths CONDITION boot2qt
    DEFINES
        SKIP_FINDEXECUTABLE
)
