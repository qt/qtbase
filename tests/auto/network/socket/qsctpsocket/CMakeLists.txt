# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsctpsocket Test:
#####################################################################

qt_internal_add_test(tst_qsctpsocket
    SOURCES
        tst_qsctpsocket.cpp
    LIBRARIES
        Qt::Network
)
