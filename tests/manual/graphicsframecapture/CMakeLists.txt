# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## graphicsframecapture Binary:
#####################################################################

qt_internal_add_manual_test(graphicsframecapture
    SOURCES
        examplewindow.cpp examplewindow.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../rhi/shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../rhi/shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set(graphicsframecapture_resource_files
    "../rhi/shared/color.frag.qsb"
    "../rhi/shared/color.vert.qsb"
)

qt_internal_add_resource(graphicsframecapture "graphicsframecapture"
    PREFIX
        "/"
    FILES
        ${graphicsframecapture_resource_files}
)
