# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qsortfilterproxymodel_regularexpression Test:
#####################################################################

qt_internal_add_test(tst_qsfpm_regex
    SOURCES
        tst_qsortfilterproxymodel_regularexpression.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::TestPrivate
)
