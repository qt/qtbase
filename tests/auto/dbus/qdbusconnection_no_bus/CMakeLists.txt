# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusconnection_no_bus Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_no_bus
    SOURCES
        tst_qdbusconnection_no_bus.cpp
    LIBRARIES
        Qt::DBus
)
