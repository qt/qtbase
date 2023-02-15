# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## clipboard Binary:
#####################################################################

qt_internal_add_manual_test(clipboard
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
# Resources:
set(data_resource_files
    "data/qticon64.png"
)

qt_internal_add_resource(clipboard "data"
    PREFIX
        "/"
    FILES
        ${data_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(clipboard CONDITION (QT_MAJOR_VERSION GREATER 4)
    LIBRARIES
        Qt::Widgets
)
