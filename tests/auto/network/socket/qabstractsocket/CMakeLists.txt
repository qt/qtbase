# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractsocket Test:
#####################################################################

qt_internal_add_test(tst_qabstractsocket
    SOURCES
        tst_qabstractsocket.cpp
    LIBRARIES
        Qt::Network
)
