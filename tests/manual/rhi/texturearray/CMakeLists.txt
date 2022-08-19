# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(texturearray
    GUI
    SOURCES
        texturearray.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set_source_files_properties("../shared/texture_arr.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture_arr.vert.qsb"
)
set_source_files_properties("../shared/texture_arr.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture_arr.frag.qsb"
)
set(texturearray_resource_files
    "../shared/texture_arr.vert.qsb"
    "../shared/texture_arr.frag.qsb"
)

qt_internal_add_resource(texturearray "texturearray"
    PREFIX
        "/"
    FILES
        ${texturearray_resource_files}
)
