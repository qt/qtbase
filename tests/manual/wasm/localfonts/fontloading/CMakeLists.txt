# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(fontloading
    GUI
    SOURCES
        main.cpp
        fontloading.html
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

add_custom_target(
    UpdateFile_fontloading ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/fontloading.html
            ${CMAKE_CURRENT_BINARY_DIR}/fontloading.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/fontloading.html
)

add_dependencies(UpdateFile_fontloading fontloading)
