# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from itemmodels.pro.

add_subdirectory(qstringlistmodel)
if(TARGET Qt::Gui)
    add_subdirectory(qabstractitemmodel)
    add_subdirectory(qabstractproxymodel)
    add_subdirectory(qconcatenatetablesproxymodel)
    add_subdirectory(qidentityproxymodel)
    add_subdirectory(qitemselectionmodel)
    add_subdirectory(qsortfilterproxymodel_recursive)
    add_subdirectory(qsortfilterproxymodel_regularexpression)
    add_subdirectory(qtransposeproxymodel)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(qsortfilterproxymodel)
endif()
if(TARGET Qt::Sql AND TARGET Qt::Widgets)
    add_subdirectory(qitemmodel)
endif()
