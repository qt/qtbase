# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractitemmodel Test:
#####################################################################

qt_internal_add_test(tst_qabstractitemmodel
    SOURCES
        ../../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qabstractitemmodel.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::Gui
)
