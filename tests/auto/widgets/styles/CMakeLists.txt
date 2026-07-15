# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(APPLE)
    add_subdirectory(qmacstyle)
endif()
if(QT_FEATURE_private_tests)
    add_subdirectory(qstyle)
endif()
if(WIN32 AND QT_FEATURE_style_windows11)
    add_subdirectory(qwindows11style)
endif()
add_subdirectory(qstyleoption)
if(QT_FEATURE_private_tests AND NOT ANDROID AND NOT QNX AND NOT UIKIT)
    add_subdirectory(qstylesheetstyle)
endif()
