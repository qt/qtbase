# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qudpsocket Test:
#####################################################################

qt_internal_add_test(tst_qudpsocket
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qudpsocket.cpp
    LIBRARIES
        Qt::Network
        Qt::TestPrivate
    QT_TEST_SERVER_LIST "danted" "echo"
)
