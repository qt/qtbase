# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt::Concurrent)
    return()
endif()
if(TARGET Qt::Widgets)
    qt_internal_add_example(imagescaling)
    qt_internal_add_example(progressdialog)
    qt_internal_add_example(runfunction)
    qt_internal_add_example(wordcount)
endif()
if(TARGET Qt::Gui)
    qt_internal_add_example(map)
endif()
