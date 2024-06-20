# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## TestIconEngine Plugin:
#####################################################################

qt_add_plugin(TestIconPlugin
    STATIC
    OUTPUT_NAME qtesticonplugin
    PLUGIN_TYPE iconengines
    main.cpp
)

qt_autogen_tools_initial_setup(TestIconPlugin)
target_link_libraries(TestIconPlugin PRIVATE Qt::Core Qt::Gui)
