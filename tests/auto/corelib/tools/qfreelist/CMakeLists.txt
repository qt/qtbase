# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfreelist Test:
#####################################################################

qt_internal_add_test(tst_qfreelist
    SOURCES
        tst_qfreelist.cpp
    LIBRARIES
        Qt::CorePrivate
)
