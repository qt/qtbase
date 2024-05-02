# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## kitchensink Binary:
#####################################################################

qt_internal_add_manual_test(kitchensink
    SOURCES
        dragwidget.cpp dragwidget.h
        main.cpp
    DEFINES
        HAVE_SCREEN_BASE_DPI
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

# Resources:
set(kitchensink_resource_files
    "qticon16.png"
    "qticon16@2x.png"
    "qticon16@3x.png"
    "qticon32.png"
    "qticon32@2x.png"
    "qticon64.png"
)

qt_internal_add_resource(kitchensink "kitchensink"
    PREFIX
        "/"
    FILES
        ${kitchensink_resource_files}
)
