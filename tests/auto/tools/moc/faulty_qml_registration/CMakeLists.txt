# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This folder is intentionally not added in the parent directory
# The project structure here is only there to support running it as
# a manual test
cmake_minimum_required(VERSION 3.16)

project(test_add_resource_options)

find_package(Qt6Core REQUIRED)

qt_wrap_cpp(faulty_registration.h)


qt6_add_executable(faulty_registration faulty_registration.h faulty_registration.cpp)
target_link_libraries(faulty_registration PRIVATE Qt::Core)

