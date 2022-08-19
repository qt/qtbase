# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

project(my_app)
cmake_minimum_required(VERSION 3.16)
find_package(Qt6 REQUIRED COMPONENTS Widgets Qml)

#! [AUTORCC]
set(CMAKE_AUTORCC ON)

qt_add_executable(my_app
    application.qrc
    main.cpp
)
#! [AUTORCC]

#! [qt_add_resources]
qt_add_resources(my_app "app_images"
    PREFIX "/"
    FILES
        images/copy.png
        images/cut.png
        images/new.png
        images/open.png
        images/paste.png
        images/save.png
)
#! [qt_add_resources]

#! [qt_add_big_resources]
qt_add_big_resources(SOURCES application.qrc)
target_sources(my_app PRIVATE ${SOURCES})
#! [qt_add_big_resources]

#! [qt_add_binary_resources]
qt_add_binary_resources(resources application.qrc DESTINATION application.rcc)
add_dependencies(my_app resources)
#! [qt_add_binary_resources]

target_link_libraries(my_app
    PRIVATE Qt6::Qml Qt6::Widgets)
