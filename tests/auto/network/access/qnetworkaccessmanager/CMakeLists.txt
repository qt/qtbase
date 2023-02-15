# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkaccessmanager Test:
#####################################################################

qt_internal_add_test(tst_qnetworkaccessmanager
    SOURCES
        tst_qnetworkaccessmanager.cpp
    LIBRARIES
        Qt::Network
)
