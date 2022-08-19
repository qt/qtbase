# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from macnativeevents.pro.

if(NOT APPLE)
    return()
endif()

#####################################################################
## tst_macnativeevents Test:
#####################################################################

qt_internal_add_test(tst_macnativeevents
    SOURCES
        expectedeventlist.cpp expectedeventlist.h
        nativeeventlist.cpp nativeeventlist.h
        qnativeevents.cpp qnativeevents.h
        qnativeevents_mac.cpp
        tst_macnativeevents.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_UP_TO=0
    LIBRARIES
        ${FWAppKit}
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:macnativeevents.pro:<TRUE>:
# _REQUIREMENTS = "mac"
