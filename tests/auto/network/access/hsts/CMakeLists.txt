# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhsts Test:
#####################################################################

qt_internal_add_test(tst_qhsts
    SOURCES
        tst_qhsts.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
)

# TEMPLATE = "app"
