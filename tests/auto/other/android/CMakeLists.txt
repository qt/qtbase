# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(permissions)
add_subdirectory(package_source_dir)

if(QT_ANDROID_TEST_APK_SPLITS)
    add_subdirectory(dynamic_feature)
endif()
if(NOT QT_ANDROID_GRADLE_MULTI_MODULE AND QT_BUILD_STANDALONE_TESTS)
    add_subdirectory(deployment_settings)
endif()
