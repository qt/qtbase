# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Include this file in your example project to use the library defined in this directory.
# This avoids find_package calls in a directory scope different from the directory scope of the
# consuming target.

if(NOT TARGET Qt::Widgets)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()

if(NOT TARGET Qt::OpenGL)
    find_package(Qt6 OPTIONAL_COMPONENTS OpenGL)
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}" painting_shared)
