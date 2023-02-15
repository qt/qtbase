# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbuslocalcalls Test:
#####################################################################

qt_internal_add_test(tst_qdbuslocalcalls
    SOURCES
        tst_qdbuslocalcalls.cpp
    LIBRARIES
        Qt::DBus
)
