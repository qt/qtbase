# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## stereo Binary:
#####################################################################

qt_internal_add_manual_test(stereo
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set(stereo_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
)

qt_internal_add_resource(stereo "stereo"
    PREFIX
        "/"
    FILES
        ${stereo_resource_files}
)
