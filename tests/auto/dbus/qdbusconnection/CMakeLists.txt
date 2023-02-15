# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusconnection Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection
    SOURCES
        tst_qdbusconnection.cpp tst_qdbusconnection.h
    LIBRARIES
        Qt::DBus
)
