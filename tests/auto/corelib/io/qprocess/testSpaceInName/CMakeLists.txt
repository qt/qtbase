# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## testSpaceInName Binary:
#####################################################################

add_executable(testSpaceInName main.cpp)
set_property(TARGET testSpaceInName PROPERTY
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../test Space In Name")
