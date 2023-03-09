# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(MockPlugins1
    NO_UNITY_BUILD
    PLUGIN_TYPES mockplugin
    SOURCES
        qmockplugin.h
        fake.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)
