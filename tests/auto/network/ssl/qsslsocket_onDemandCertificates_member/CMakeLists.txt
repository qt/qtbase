# Generated from qsslsocket_onDemandCertificates_member.pro.

#####################################################################
## tst_qsslsocket_onDemandCertificates_member Test:
#####################################################################

add_qt_test(tst_qsslsocket_onDemandCertificates_member
    GUI
    SOURCES
        tst_qsslsocket_onDemandCertificates_member.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qsslsocket_onDemandCertificates_member.pro:<TRUE>:
# CONFIG = "testcase"
# testcase.timeout = "300"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:qsslsocket_onDemandCertificates_member.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "debug"

#### Keys ignored in scope 4:.:.:qsslsocket_onDemandCertificates_member.pro:else:
# DESTDIR = "release"

#### Keys ignored in scope 5:.:.:qsslsocket_onDemandCertificates_member.pro:LINUX:
# CONFIG = "unsupported/testserver"
# QT_TEST_SERVER_LIST = "squid" "danted"
