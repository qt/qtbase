# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsystemtrayicon.pro.

#####################################################################
## qsystemtrayicon Binary:
#####################################################################

qt_internal_add_manual_test(qsystemtrayicon
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
set(icons_resource_files
    "macsystray16x16.png"
    "macsystray18x18.png"
    "macsystray25x15.png"
    "macsystray32x32.png"
    "macsystray36x36.png"
    "macsystray50x30.png"
    "macsystray64x64.png"
)

qt_internal_add_resource(qsystemtrayicon "icons"
    PREFIX
        "/"
    FILES
        ${icons_resource_files}
)


#### Keys ignored in scope 1:.:.:qsystemtrayicon.pro:<TRUE>:
# TEMPLATE = "app"
