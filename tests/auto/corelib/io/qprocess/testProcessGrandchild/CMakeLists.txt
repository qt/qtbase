# Copyright (C) 2025 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testProcessNormal Binary:
#####################################################################

qt_internal_add_executable(testProcessGrandchild
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)
