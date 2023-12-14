# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(fontloading
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

add_custom_command(
    TARGET fontloading POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/fontloading.html
            ${CMAKE_CURRENT_BINARY_DIR}/fontloading.html)
#add_custom_target(html DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fontloading.html)
#add_dependencies(fontloading html)
