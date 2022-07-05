# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtcpsocket.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()
add_subdirectory(test)
if(NOT VXWORKS)
    add_subdirectory(stressTest)
endif()
