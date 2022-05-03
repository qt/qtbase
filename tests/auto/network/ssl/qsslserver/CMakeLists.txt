if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsslserver Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

qt_internal_add_test(tst_qsslserver
    SOURCES
        tst_qsslserver.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    TESTDATA ${test_data}
)
