# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(pem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_add_executable(pem
    main.cpp
)

target_link_libraries(pem PUBLIC
    Qt::Core
    Qt::Network
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(pem PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(pem PRIVATE
        -fsanitize=fuzzer
    )
endif()
