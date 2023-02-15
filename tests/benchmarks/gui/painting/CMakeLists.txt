# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(drawtexture)
add_subdirectory(qcolor)
add_subdirectory(qregion)
add_subdirectory(qtransform)
add_subdirectory(lancebench)
if(TARGET Qt::Widgets)
    add_subdirectory(qpainter)
    add_subdirectory(qtbench)
endif()
