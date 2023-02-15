# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkproxy Test:
#####################################################################

qt_internal_add_test(tst_qnetworkproxy
    SOURCES
        tst_qnetworkproxy.cpp
    LIBRARIES
        Qt::Network
)
