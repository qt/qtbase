# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT APPLE)
    return()
endif()

qt_internal_add_test(tst_qaccessibilitymac
    SOURCES
        tst_qaccessibilitymac.mm
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWSecurity}
)
