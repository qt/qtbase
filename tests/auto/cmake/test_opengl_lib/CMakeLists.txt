# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_opengl_lib)

find_package(Qt6Gui REQUIRED)
find_package(Qt6OpenGL REQUIRED)

get_property(QtGui_enabled_features TARGET Qt6::Gui PROPERTY QT_ENABLED_PUBLIC_FEATURES)
list(LENGTH QtGui_enabled_features _QtGui_enabled_featuresSize)
if (_QtGui_enabled_featuresSize LESS 1)
    message(SEND_ERROR "The QT_ENABLED_PUBLIC_FEATURES property of Qt6::Gui is empty!")
endif()

add_executable(myobject main.cpp )
target_link_libraries(myobject Qt::Gui Qt::OpenGL)
