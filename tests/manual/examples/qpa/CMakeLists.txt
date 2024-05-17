# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt6::Gui)
    return()
endif()
qt_internal_add_example(windows)
qt_internal_add_example(qrasterwindow)
