# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractproxymodel Test:
#####################################################################

qt_internal_add_test(tst_qabstractproxymodel
    SOURCES
        tst_qabstractproxymodel.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
)
