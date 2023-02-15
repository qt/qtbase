# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qitemselectionmodel Test:
#####################################################################

qt_internal_add_test(tst_qitemselectionmodel
    SOURCES
        tst_qitemselectionmodel.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
)
