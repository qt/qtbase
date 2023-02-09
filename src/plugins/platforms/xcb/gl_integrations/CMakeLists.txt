# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_xcb_egl_plugin)
    add_subdirectory(xcb_egl)
endif()
if(QT_FEATURE_xcb_glx_plugin)
    add_subdirectory(xcb_glx)
endif()
