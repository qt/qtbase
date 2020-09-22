# Generated from test.pro.

#####################################################################
## tst_qtcpserver Test:
#####################################################################

qt_internal_add_test(tst_qtcpserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_qtcpserver.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    QT_TEST_SERVER_LIST "danted" "cyrus" "squid" "ftp-proxy" # special case
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtcpserver CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

#### Keys ignored in scope 6:.:.:test.pro:LINUX:
# QT_TEST_SERVER_LIST = "danted" "cyrus" "squid" "ftp-proxy"
