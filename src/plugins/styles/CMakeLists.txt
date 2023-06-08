# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_style_android)
    add_subdirectory(android)
endif()
if(QT_FEATURE_style_mac)
    add_subdirectory(mac)
endif()
if(QT_FEATURE_style_windowsvista)
    add_subdirectory(modernwindows)
endif()
