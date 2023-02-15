# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhooks Test:
#####################################################################

qt_internal_add_test(tst_qhooks
    SOURCES
        tst_qhooks.cpp
    LIBRARIES
        Qt::CorePrivate
)
