# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_networkselftest Test:
#####################################################################

qt_internal_add_test(tst_networkselftest
    SOURCES
        tst_networkselftest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
    QT_TEST_SERVER_LIST "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "cyrus" "echo"
)
