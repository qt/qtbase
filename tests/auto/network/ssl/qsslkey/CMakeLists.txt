# Generated from qsslkey.pro.

#####################################################################
## tst_qsslkey Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    keys/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    rsa-*.pem)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qsslkey
    SOURCES
        tst_qsslkey.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsslkey CONDITION QT_FEATURE_private_tests
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)
