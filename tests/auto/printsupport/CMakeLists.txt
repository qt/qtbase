# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_printer OR NOT QT_FEATURE_printdialog)
    return()
endif()
add_subdirectory(dialogs)
add_subdirectory(kernel)
