# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qstringlistmodel)
if(TARGET Qt::Gui)
    add_subdirectory(qabstractitemmodel)
    if(QT_FEATURE_proxymodel)
        if (NOT WASM) # QTBUG-121822
        add_subdirectory(qabstractproxymodel)
        endif()
        add_subdirectory(qconcatenatetablesproxymodel)
        add_subdirectory(qidentityproxymodel)
        add_subdirectory(qsortfilterproxymodel_recursive)
        add_subdirectory(qsortfilterproxymodel_regularexpression)
        add_subdirectory(qtransposeproxymodel)
    endif()
    add_subdirectory(qitemselectionmodel)
endif()
if(TARGET Qt::Widgets AND QT_FEATURE_proxymodel)
    add_subdirectory(qsortfilterproxymodel)
endif()
if(TARGET Qt::Sql AND TARGET Qt::Widgets)
    add_subdirectory(qitemmodel)
endif()
