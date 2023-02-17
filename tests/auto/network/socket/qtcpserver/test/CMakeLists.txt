# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtcpserver Test:
#####################################################################

qt_internal_add_test(tst_qtcpserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qtcpserver.cpp
    LIBRARIES
        Qt::Network
    QT_TEST_SERVER_LIST "danted" "cyrus" "squid" "ftp-proxy"
)

add_dependencies(tst_qtcpserver
    crashingServer
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtcpserver CONDITION WIN32
    LIBRARIES
        ws2_32
)
