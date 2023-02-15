# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()
add_subdirectory(test)
if(NOT VXWORKS)
    add_subdirectory(stressTest)
endif()
