# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qprintdevice_dump Binary:
#####################################################################

qt_internal_add_manual_test(qprintdevice_dump
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::PrintSupportPrivate
)
