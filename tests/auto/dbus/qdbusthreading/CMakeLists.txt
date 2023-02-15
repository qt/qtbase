# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusthreading Test:
#####################################################################

qt_internal_add_test(tst_qdbusthreading
    SOURCES
        tst_qdbusthreading.cpp
    LIBRARIES
        Qt::DBus
)
