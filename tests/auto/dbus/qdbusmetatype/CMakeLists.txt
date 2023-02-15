# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusmetatype Test:
#####################################################################

qt_internal_add_test(tst_qdbusmetatype
    SOURCES
        tst_qdbusmetatype.cpp
    LIBRARIES
        Qt::DBus
)
