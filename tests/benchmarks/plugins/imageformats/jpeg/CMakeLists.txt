# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## jpeg Binary:
#####################################################################

qt_internal_add_benchmark(jpeg
    SOURCES
        jpeg.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)
