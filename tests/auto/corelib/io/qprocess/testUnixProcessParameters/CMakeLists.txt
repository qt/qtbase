# Copyright (C) 2023 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testProcessNormal Binary:
#####################################################################

qt_internal_add_executable(testUnixProcessParameters
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    CORE_LIBRARY None
    SOURCES
        main.cpp
)
