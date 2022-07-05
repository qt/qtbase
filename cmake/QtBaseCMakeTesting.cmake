# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

## Test the cmake build system:
option(BUILD_CMAKE_TESTING "Build tests for the Qt build system" OFF)
mark_as_advanced(BUILD_CMAKE_TESTING)

if(BUILD_CMAKE_TESTING)
    add_subdirectory("${PROJECT_SOURCE_DIR}/cmake/tests")
endif()


