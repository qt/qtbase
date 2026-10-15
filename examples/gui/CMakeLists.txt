# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt6::Gui)
    return()
endif()
add_subdirectory(platform)
qt_internal_add_example(rasterwindow)
qt_internal_add_example(rhiwindow)
