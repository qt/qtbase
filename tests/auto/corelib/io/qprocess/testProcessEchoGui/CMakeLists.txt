# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## testProcessEchoGui Binary:
#####################################################################

add_executable(testProcessEchoGui WIN32 main_win.cpp)
target_link_libraries(testProcessEchoGui PRIVATE user32)
