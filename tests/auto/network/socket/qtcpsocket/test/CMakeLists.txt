# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtcpsocket Test:
#####################################################################

qt_internal_add_test(tst_qtcpsocket
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qtcpsocket.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "iptables" "cyrus"
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtcpsocket CONDITION WIN32
    LIBRARIES
        ws2_32
)
