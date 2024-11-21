# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusconnection_signalorder Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_signalorder
    SOURCES
        tst_qdbusconnection_signalorder.cpp
    LIBRARIES
        Qt::DBus
)
