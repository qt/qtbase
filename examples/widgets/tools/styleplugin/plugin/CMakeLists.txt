# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_plugin(simplestyleplugin
    CLASS_NAME SimpleStylePlugin
    simplestyle.cpp simplestyle.h
    simplestyleplugin.cpp simplestyleplugin.h
)

set_target_properties(simplestyleplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/stylewindow/styles"
)

target_link_libraries(simplestyleplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS simplestyleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
