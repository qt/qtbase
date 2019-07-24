# Generated from test.pro.

#####################################################################
## tst_qtcpserver Test:
#####################################################################

add_qt_test(tst_qtcpserver
    GUI
    SOURCES
        ../tst_qtcpserver.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# CONFIG = "testcase"
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

extend_target(tst_qtcpserver CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

#### Keys ignored in scope 6:.:.:test.pro:LINUX:
# CONFIG = "unsupported/testserver"
# QT_TEST_SERVER_LIST = "danted" "cyrus" "squid" "ftp-proxy"
