# Generated from qabstractnetworkcache.pro.

#####################################################################
## tst_qabstractnetworkcache Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "tests/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qabstractnetworkcache
    GUI
    SOURCES
        tst_qabstractnetworkcache.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qabstractnetworkcache.pro:<TRUE>:
# CONFIG = "testcase" "unsupported/testserver"
# QT_TEST_SERVER_LIST = "apache2"
