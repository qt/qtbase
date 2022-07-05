# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from painting.pro.

add_subdirectory(drawtexture)
add_subdirectory(qcolor)
add_subdirectory(qregion)
add_subdirectory(qtransform)
add_subdirectory(lancebench)
if(TARGET Qt::Widgets)
    add_subdirectory(qpainter)
    add_subdirectory(qtbench)
endif()
