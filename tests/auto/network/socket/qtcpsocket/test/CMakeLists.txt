# Generated from test.pro.

#####################################################################
## tst_qtcpsocket Test:
#####################################################################

add_qt_test(tst_qtcpsocket
    GUI
    SOURCES
        ../tst_qtcpsocket.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qtcpsocket CONDITION WIN32
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
# CONFIG = "unsupported/testserver"
# QT_TEST_SERVER_LIST = "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "iptables" "cyrus"
