# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from test.pro.

#####################################################################
## tst_qtcpserver Test:
#####################################################################

qt_internal_add_test(tst_qtcpserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_qtcpserver.cpp
    LIBRARIES
        Qt::Network
        Qt::TestPrivate
    QT_TEST_SERVER_LIST "danted" "cyrus" "squid" "ftp-proxy" # special case
)

add_dependencies(tst_qtcpserver
    crashingServer
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtcpserver CONDITION WIN32
    LIBRARIES
        ws2_32
)

#### Keys ignored in scope 6:.:.:test.pro:LINUX:
# QT_TEST_SERVER_LIST = "danted" "cyrus" "squid" "ftp-proxy"
