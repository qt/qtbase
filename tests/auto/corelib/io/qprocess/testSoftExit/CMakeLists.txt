# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testSoftExit Binary:
#####################################################################

add_executable(testSoftExit)

qt_internal_extend_target(testSoftExit CONDITION WIN32
    SOURCES
        main_win.cpp
    LIBRARIES
        user32
)

qt_internal_extend_target(testSoftExit CONDITION UNIX
    SOURCES
        main_unix.cpp
)
