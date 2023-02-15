# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## scrolltest Binary:
#####################################################################

qt_internal_add_benchmark(scrolltest
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)
