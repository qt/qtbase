# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testProcessEchoGui Binary:
#####################################################################

qt_internal_add_executable(testProcessEchoGui
    GUI
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    CORE_LIBRARY None
    SOURCES
        main_win.cpp
)
target_link_libraries(testProcessEchoGui PRIVATE user32)
