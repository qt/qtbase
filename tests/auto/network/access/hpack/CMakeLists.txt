# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_hpack Test:
#####################################################################

qt_internal_add_test(tst_hpack
    SOURCES
        tst_hpack.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
)
