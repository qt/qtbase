# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(ANDROID)
    add_subdirectory(android)
    add_subdirectory(android_appless)
    add_subdirectory(androiditemmodel)
    add_subdirectory(android_legacy_packaging)
endif()
if(WIN32)
    add_subdirectory(windows)
endif()
