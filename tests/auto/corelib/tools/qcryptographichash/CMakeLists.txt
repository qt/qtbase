# Generated from qcryptographichash.pro.

#####################################################################
## tst_qcryptographichash Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qcryptographichash
    SOURCES
        tst_qcryptographichash.cpp
    TESTDATA ${test_data}
)
