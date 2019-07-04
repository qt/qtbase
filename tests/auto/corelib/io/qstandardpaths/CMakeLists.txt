# Generated from qstandardpaths.pro.

#####################################################################
## tst_qstandardpaths Test:
#####################################################################

add_qt_test(tst_qstandardpaths
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qstandardpaths.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
)

#### Keys ignored in scope 1:.:.:qstandardpaths.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "tst_qstandardpaths.cpp" "qstandardpaths.pro"

## Scopes:
#####################################################################

extend_target(tst_qstandardpaths CONDITION boot2qt
    DEFINES
        SKIP_FINDEXECUTABLE
)
