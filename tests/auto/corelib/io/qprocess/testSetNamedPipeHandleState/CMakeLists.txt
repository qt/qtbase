# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testSetNamedPipeHandleState Binary:
#####################################################################

add_executable(testSetNamedPipeHandleState main.cpp)
target_link_libraries(testSetNamedPipeHandleState PRIVATE kernel32.lib)
