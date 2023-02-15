# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qvulkaninstance Binary:
#####################################################################

qt_internal_add_manual_test(qvulkaninstance
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
