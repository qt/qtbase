# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_dependent_modules)

# The module finds its dependencies
find_package(Qt6Widgets REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_wrap_cpp(moc_files mywidget.h)
qt_wrap_ui(ui_files mywidget.ui)

add_executable(mywidget mywidget.cpp ${moc_files} ${ui_files})
target_link_libraries(mywidget Qt::Widgets)
