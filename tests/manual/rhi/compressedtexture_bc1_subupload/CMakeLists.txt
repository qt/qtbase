# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## compressedtexture_bc1_subupload Binary:
#####################################################################

qt_internal_add_manual_test(compressedtexture_bc1_subupload
    GUI
    SOURCES
        compressedtexture_bc1_subupload.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/bwqt224_64_nomips.dds"
    PROPERTIES QT_RESOURCE_ALIAS "bwqt224_64_nomips.dds"
)
set_source_files_properties("../shared/qt256_bc1_9mips.dds"
    PROPERTIES QT_RESOURCE_ALIAS "qt256_bc1_9mips.dds"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(compressedtexture_bc1_subupload_resource_files
    "../shared/bwqt224_64_nomips.dds"
    "../shared/qt256_bc1_9mips.dds"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
)

qt_internal_add_resource(compressedtexture_bc1_subupload "compressedtexture_bc1_subupload"
    PREFIX
        "/"
    FILES
        ${compressedtexture_bc1_subupload_resource_files}
)
