# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from gl_integrations.pro.

if(QT_FEATURE_xcb_egl_plugin)
    add_subdirectory(xcb_egl)
endif()
if(QT_FEATURE_xcb_glx_plugin)
    add_subdirectory(xcb_glx)
endif()
