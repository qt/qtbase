# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbuspendingcall Test:
#####################################################################

qt_internal_add_test(tst_qdbuspendingcall
    SOURCES
        tst_qdbuspendingcall.cpp
    LIBRARIES
        Qt::DBus
)
