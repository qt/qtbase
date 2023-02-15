# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusconnection_delayed Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_delayed
    SOURCES
        tst_qdbusconnection_delayed.cpp
    LIBRARIES
        Qt::DBus
)
