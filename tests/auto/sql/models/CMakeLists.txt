# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from models.pro.

add_subdirectory(qsqlrelationaltablemodel)
add_subdirectory(qsqltablemodel)
if(TARGET Qt::Widgets)
    add_subdirectory(qsqlquerymodel)
    add_subdirectory(qsqlrelationaldelegate)
endif()
