# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from test.pro.

#####################################################################
## tst_qclipboard Test:
#####################################################################

qt_internal_add_test(tst_qclipboard
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qclipboard.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qclipboard CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
)

#### Keys ignored in scope 6:.:.:test.pro:NOT ANDROID:
# TEST_HELPER_INSTALLS = "../copier/copier" "../paster/paster"
