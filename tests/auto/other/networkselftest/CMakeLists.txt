# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from networkselftest.pro.

#####################################################################
## tst_networkselftest Test:
#####################################################################

qt_internal_add_test(tst_networkselftest
    SOURCES
        tst_networkselftest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
    QT_TEST_SERVER_LIST "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "cyrus" "echo" # special case
)

#### Keys ignored in scope 1:.:.:networkselftest.pro:<TRUE>:
# QT_TEST_SERVER_LIST = "cyrus" "vsftpd" "apache2" "ftp-proxy" "danted" "squid" "echo"
