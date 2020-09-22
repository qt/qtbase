# Generated from test.pro.

#####################################################################
## tst_qtcpsocket Test:
#####################################################################

qt_internal_add_test(tst_qtcpsocket
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_qtcpsocket.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "iptables" "cyrus" # special case
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtcpsocket CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

#### Keys ignored in scope 4:.:.:test.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug"

#### Keys ignored in scope 5:.:.:test.pro:else:
# DESTDIR = "../release"

#### Keys ignored in scope 6:.:.:test.pro:else:
# DESTDIR = "../"

#### Keys ignored in scope 7:.:.:test.pro:LINUX:
# QT_TEST_SERVER_LIST = "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "iptables" "cyrus"
