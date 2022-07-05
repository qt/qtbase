# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qabstractitemmodel.pro.

#####################################################################
## tst_qabstractitemmodel Test:
#####################################################################

qt_internal_add_test(tst_qabstractitemmodel
    SOURCES
        ../../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qabstractitemmodel.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::Gui
)
