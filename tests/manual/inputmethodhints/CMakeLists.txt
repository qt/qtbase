# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_inputmethodhints Binary:
#####################################################################

qt_internal_add_manual_test(tst_inputmethodhints
    GUI
    SOURCES
        inputmethodhints.cpp inputmethodhints.h inputmethodhints.ui
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
