# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET Qt6::Gui OR (NOT embedded AND NOT x11))
    return()
endif()
qt_internal_add_example(styleexample)
qt_internal_add_example(raycasting)
qt_internal_add_example(flickable)
qt_internal_add_example(digiflip)
qt_internal_add_example(lightmaps)
qt_internal_add_example(flightinfo)
