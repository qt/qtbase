# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qt-client-raster Binary:
#####################################################################

qt_internal_add_manual_test(qt-client-raster
    GUI
    SOURCES
        main.cpp
        rasterwindow.cpp rasterwindow.h
    LIBRARIES
        Qt::Gui
)
