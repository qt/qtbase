# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from printsupport.pro.

if(NOT QT_FEATURE_printer OR NOT QT_FEATURE_printdialog)
    return()
endif()
add_subdirectory(dialogs)
add_subdirectory(kernel)
