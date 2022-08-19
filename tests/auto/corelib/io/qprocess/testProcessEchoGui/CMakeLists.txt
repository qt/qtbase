# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testProcessEchoGui Binary:
#####################################################################

add_executable(testProcessEchoGui WIN32 main_win.cpp)
target_link_libraries(testProcessEchoGui PRIVATE user32)
