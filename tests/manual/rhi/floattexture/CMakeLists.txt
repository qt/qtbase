# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## floattexture Binary:
#####################################################################

qt_internal_add_manual_test(floattexture
    GUI
    SOURCES
        floattexture.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/OpenfootageNET_fieldairport-512.hdr"
    PROPERTIES QT_RESOURCE_ALIAS "OpenfootageNET_fieldairport-512.hdr"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(floattexture_resource_files
    "../shared/OpenfootageNET_fieldairport-512.hdr"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
)

qt_internal_add_resource(floattexture "floattexture"
    PREFIX
        "/"
    FILES
        ${floattexture_resource_files}
)


# TEMPLATE = "app"
