# Generated from qsslkey.pro.

#####################################################################
## tst_qsslkey Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "keys/*")
list(APPEND test_data ${test_data_glob})

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "rsa-*.pem")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qsslkey
    GUI
    SOURCES
        tst_qsslkey.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qsslkey.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qsslkey CONDITION QT_FEATURE_private_tests
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
)
