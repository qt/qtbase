# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## moveItems Binary:
#####################################################################

qt_internal_add_benchmark(moveItems
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)
