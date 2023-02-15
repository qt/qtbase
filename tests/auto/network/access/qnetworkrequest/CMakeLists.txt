# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkrequest Test:
#####################################################################

qt_internal_add_test(tst_qnetworkrequest
    SOURCES
        tst_qnetworkrequest.cpp
    LIBRARIES
        Qt::Network
)
