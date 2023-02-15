# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## computeimage Binary:
#####################################################################

qt_internal_add_manual_test(computeimage
    GUI
    SOURCES
        computeimage.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/qt256.png"
    PROPERTIES QT_RESOURCE_ALIAS "qt256.png"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(computeimage_resource_files
    "../shared/qt256.png"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
    "image.comp.qsb"
)

qt_internal_add_resource(computeimage "computeimage"
    PREFIX
        "/"
    FILES
        ${computeimage_resource_files}
)
