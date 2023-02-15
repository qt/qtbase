# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## cubemap_scissor Binary:
#####################################################################

qt_internal_add_manual_test(cubemap_scissor
    GUI
    SOURCES
        cubemap_scissor.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../cubemap/c.png"
    PROPERTIES QT_RESOURCE_ALIAS "c.png"
)
set_source_files_properties("../cubemap/cubemap.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "cubemap.frag.qsb"
)
set_source_files_properties("../cubemap/cubemap.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "cubemap.vert.qsb"
)
set(cubemap_scissor_resource_files
    "../cubemap/c.png"
    "../cubemap/cubemap.frag.qsb"
    "../cubemap/cubemap.vert.qsb"
)

qt_internal_add_resource(cubemap_scissor "cubemap_scissor"
    PREFIX
        "/"
    FILES
        ${cubemap_scissor_resource_files}
)
