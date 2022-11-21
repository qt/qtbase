# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_plugin(echoplugin
    CLASS_NAME EchoPlugin
    echoplugin.cpp echoplugin.h
)

set_target_properties(echoplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/echowindow"
)

target_include_directories(echoplugin PRIVATE
    ../echowindow
)

target_link_libraries(echoplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS echoplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
