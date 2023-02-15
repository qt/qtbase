# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## html2md Binary:
#####################################################################

qt_internal_add_manual_test(html2md
    GUI
    SOURCES
        html2md.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
)
