# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qsqlrelationaltablemodel)
add_subdirectory(qsqltablemodel)
if(TARGET Qt::Widgets)
    add_subdirectory(qsqlquerymodel)
    add_subdirectory(qsqlrelationaldelegate)
endif()
