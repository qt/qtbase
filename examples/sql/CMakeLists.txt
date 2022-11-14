# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET Qt6::Widgets)
    return()
endif()
qt_internal_add_example(books)
qt_internal_add_example(drilldown)
qt_internal_add_example(cachedtable)
qt_internal_add_example(querymodel)
qt_internal_add_example(relationaltablemodel)
qt_internal_add_example(sqlwidgetmapper)
qt_internal_add_example(tablemodel)
if(TARGET Qt6::Xml)
    qt_internal_add_example(masterdetail)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    qt_internal_add_example(sqlbrowser)
endif()
