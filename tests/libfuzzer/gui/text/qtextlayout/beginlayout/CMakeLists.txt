# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(beginlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

qt_add_executable(beginlayout
    main.cpp
)

target_link_libraries(beginlayout PUBLIC
    Qt::Core
    Qt::Gui
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(beginlayout PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(beginlayout PRIVATE
        -fsanitize=fuzzer
    )
endif()

qt_import_plugins(beginlayout
    INCLUDE Qt::QMinimalIntegrationPlugin
)
