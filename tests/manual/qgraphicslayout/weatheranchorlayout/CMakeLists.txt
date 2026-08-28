# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(weatheranchorlayout LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## weatheranchorlayout Binary:
#####################################################################
cmake_minimum_required(VERSION 3.16)

qt_internal_add_manual_test(weatheranchorlayout
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
set(weatheranchorlayout_resource_files
    "images/5days.jpg"
    "images/details.jpg"
    "images/place.jpg"
    "images/tabbar.jpg"
    "images/title.jpg"
    "images/weather-few-clouds.png"
)

qt_add_resources(weatheranchorlayout "weatheranchorlayout"
    PREFIX
        "/"
    FILES
        ${weatheranchorlayout_resource_files}
)

