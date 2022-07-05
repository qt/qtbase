# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from styles.pro.

if(APPLE)
    add_subdirectory(qmacstyle)
endif()
if(QT_FEATURE_private_tests)
    add_subdirectory(qstyle)
endif()
add_subdirectory(qstyleoption)
if(QT_FEATURE_private_tests AND NOT ANDROID AND NOT QNX AND NOT UIKIT)
    add_subdirectory(qstylesheetstyle)
endif()
