# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qloggingcategory Test:
#####################################################################

qt_internal_add_test(tst_qloggingcategory
    SOURCES
        tst_qloggingcategory.cpp
    LIBRARIES
        Qt::CorePrivate
)
