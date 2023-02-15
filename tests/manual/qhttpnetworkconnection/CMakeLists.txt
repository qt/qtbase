# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_qhttpnetworkconnection Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_qhttpnetworkconnection
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)
