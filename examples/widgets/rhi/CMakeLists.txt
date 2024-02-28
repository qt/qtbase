# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt6::Widgets)
    return()
endif()
qt_internal_add_example(simplerhiwidget)
qt_internal_add_example(cuberhiwidget)
