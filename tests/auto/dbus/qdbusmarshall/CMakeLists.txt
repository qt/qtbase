# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()
add_subdirectory(qdbusmarshall)
add_subdirectory(qpong)
add_dependencies(tst_qdbusmarshall qpong)
