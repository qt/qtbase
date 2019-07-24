# Generated from qsslsocket.pro.

#####################################################################
## tst_qsslsocket Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

add_qt_test(tst_qsslsocket
    GUI
    SOURCES
        tst_qsslsocket.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qsslsocket.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:qsslsocket.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "debug"

#### Keys ignored in scope 4:.:.:qsslsocket.pro:else:
# DESTDIR = "release"

#### Keys ignored in scope 5:.:.:qsslsocket.pro:LINUX:
# CONFIG = "unsupported/testserver"
# QT_TEST_SERVER_LIST = "squid" "danted" "cyrus" "apache2" "echo"
