# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusreply Test:
#####################################################################

qt_internal_add_test(tst_qdbusreply
    SOURCES
        tst_qdbusreply.cpp
    LIBRARIES
        Qt::DBus
)
