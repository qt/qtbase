# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## mrt Binary:
#####################################################################

qt_internal_add_manual_test(mrt
    GUI
    SOURCES
        mrt.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(mrt_resource_files
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
    "mrt.frag.qsb"
    "mrt.vert.qsb"
)

qt_internal_add_resource(mrt "mrt"
    PREFIX
        "/"
    FILES
        ${mrt_resource_files}
)
