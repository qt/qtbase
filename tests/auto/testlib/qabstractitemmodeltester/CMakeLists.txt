# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qabstractitemmodeltester.pro.

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
