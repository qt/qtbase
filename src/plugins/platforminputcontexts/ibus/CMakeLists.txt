# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QIbusPlatformInputContextPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QIbusPlatformInputContextPlugin
    OUTPUT_NAME ibusplatforminputcontextplugin
    PLUGIN_TYPE platforminputcontexts
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qibusinputcontextproxy.cpp qibusinputcontextproxy.h
        qibusplatforminputcontext.cpp qibusplatforminputcontext.h
        qibusproxy.cpp qibusproxy.h
        qibusproxyportal.cpp qibusproxyportal.h
        qibustypes.cpp qibustypes.h
    LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
        XKB::XKB
)
