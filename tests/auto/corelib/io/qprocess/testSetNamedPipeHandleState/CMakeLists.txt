# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testSetNamedPipeHandleState Binary:
#####################################################################

qt_internal_add_executable(testSetNamedPipeHandleState
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    CORE_LIBRARY None
    SOURCES
        main.cpp
)
target_link_libraries(testSetNamedPipeHandleState PRIVATE kernel32.lib)
