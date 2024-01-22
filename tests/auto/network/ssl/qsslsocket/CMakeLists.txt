# Generated from qsslsocket.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsslsocket Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

qt_internal_add_test(tst_qsslsocket
    SOURCES
        tst_qsslsocket.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
    QT_TEST_SERVER_LIST "squid" "danted" "cyrus" "apache2" "echo" # special case
)

qt_internal_extend_target(tst_qsslsocket CONDITION QT_FEATURE_private_tests AND QT_FEATURE_openssl_linked
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)

#### Keys ignored in scope 1:.:.:qsslsocket.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:qsslsocket.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "debug"

#### Keys ignored in scope 4:.:.:qsslsocket.pro:else:
# DESTDIR = "release"

#### Keys ignored in scope 5:.:.:qsslsocket.pro:LINUX:
# QT_TEST_SERVER_LIST = "squid" "danted" "cyrus" "apache2" "echo"
