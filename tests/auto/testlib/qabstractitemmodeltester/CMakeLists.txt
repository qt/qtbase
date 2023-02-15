# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractitemmodeltester Test:
#####################################################################

qt_internal_add_test(tst_qabstractitemmodeltester
    SOURCES
        ../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qabstractitemmodeltester.cpp
    INCLUDE_DIRECTORIES
        ../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
