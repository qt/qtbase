# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(totime LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(totime
    totime.cpp
)

target_link_libraries(totime PUBLIC
    Qt::Core
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(totime PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(totime PRIVATE
        -fsanitize=fuzzer
    )
endif()
