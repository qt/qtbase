# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(ANDROID)
    add_subdirectory(android)
    add_subdirectory(androidnativesurfaces)
    add_subdirectory(android_appless)
    add_subdirectory(androiditemmodel)
    add_subdirectory(androidactivityalias)
    add_subdirectory(android_packaging_mode)
endif()
if(OHOS)
    add_subdirectory(harmony)
endif()
if(WIN32)
    add_subdirectory(windows)
endif()
