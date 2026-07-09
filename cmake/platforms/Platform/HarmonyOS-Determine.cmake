# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# uname -p returns "unknown" on HarmonyOS; use uname -m for the real arch.
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "unknown")
    execute_process(
        COMMAND ${CMAKE_UNAME} -m
        OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_PROCESSOR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()
