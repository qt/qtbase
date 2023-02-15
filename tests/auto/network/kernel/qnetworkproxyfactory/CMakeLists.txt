# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkproxyfactory Test:
#####################################################################

qt_internal_add_test(tst_qnetworkproxyfactory
    SOURCES
        tst_qnetworkproxyfactory.cpp
    LIBRARIES
        Qt::NetworkPrivate
)
