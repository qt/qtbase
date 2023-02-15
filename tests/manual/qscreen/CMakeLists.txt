# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qscreen Binary:
#####################################################################

qt_internal_add_manual_test(qscreen
    SOURCES
        main.cpp
        propertyfield.cpp propertyfield.h
        propertywatcher.cpp propertywatcher.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
