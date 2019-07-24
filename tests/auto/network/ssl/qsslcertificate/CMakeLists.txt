# Generated from qsslcertificate.pro.

#####################################################################
## tst_qsslcertificate Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "certificates/*")
list(APPEND test_data ${test_data_glob})

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "more-certificates/*")
list(APPEND test_data ${test_data_glob})

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "verify-certs/*")
list(APPEND test_data ${test_data_glob})

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "pkcs12/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qsslcertificate
    GUI
    SOURCES
        tst_qsslcertificate.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qsslcertificate.pro:<TRUE>:
# CONFIG = "testcase"
