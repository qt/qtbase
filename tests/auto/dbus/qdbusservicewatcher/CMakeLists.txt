# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusservicewatcher Test:
#####################################################################

qt_internal_add_test(tst_qdbusservicewatcher
    SOURCES
        tst_qdbusservicewatcher.cpp
    LIBRARIES
        Qt::DBus
        Qt::TestPrivate
)
