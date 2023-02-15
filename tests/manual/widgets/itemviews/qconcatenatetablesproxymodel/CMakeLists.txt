# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qconcatenatetablesproxymodel Binary:
#####################################################################

qt_internal_add_manual_test(qconcatenatetablesproxymodel
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
