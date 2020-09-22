# Generated from qmake.pro.

#####################################################################
## tst_qmake Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmake
    SOURCES
        testcompiler.cpp testcompiler.h
        tst_qmake.cpp
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmake.pro:WIN32:
# testcase.timeout = "900"
