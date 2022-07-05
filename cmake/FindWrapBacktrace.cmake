# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if(TARGET WrapBacktrace::WrapBacktrace)
    set(WrapBacktrace_FOUND ON)
    return()
endif()

find_package(Backtrace)

if(Backtrace_FOUND)
    add_library(WrapBacktrace::WrapBacktrace INTERFACE IMPORTED)
    target_link_libraries(WrapBacktrace::WrapBacktrace
                          INTERFACE ${Backtrace_LIBRARY})
    target_include_directories(WrapBacktrace::WrapBacktrace
                               INTERFACE ${Backtrace_INCLUDE_DIR})
    set(WrapBacktrace_FOUND ON)
else()
    set(WrapBacktrace_FOUND OFF)
endif()
