# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## testSetNamedPipeHandleState Binary:
#####################################################################

add_executable(testSetNamedPipeHandleState main.cpp)
target_link_libraries(testSetNamedPipeHandleState PRIVATE kernel32.lib)
