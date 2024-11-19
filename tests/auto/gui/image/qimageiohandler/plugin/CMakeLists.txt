# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## TestImagePlugin:
#####################################################################

qt_add_plugin(TestImagePlugin
    STATIC
    OUTPUT_NAME qtestimageplugin
    PLUGIN_TYPE imageformats
    main.cpp
    ../pluginlog.h
)

qt_autogen_tools_initial_setup(TestImagePlugin)
target_link_libraries(TestImagePlugin PRIVATE Qt::Core Qt::Gui)
