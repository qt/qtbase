# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## triquadcube Binary:
#####################################################################

qt_internal_add_manual_test(triquadcube
    GUI
    SOURCES
        quadrenderer.cpp quadrenderer.h
        texturedcuberenderer.cpp texturedcuberenderer.h
        triangleoncuberenderer.cpp triangleoncuberenderer.h
        trianglerenderer.cpp trianglerenderer.h
        triquadcube.cpp
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
set_source_files_properties("../shared/qt256.png"
    PROPERTIES QT_RESOURCE_ALIAS "qt256.png"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(triquadcube_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
    "../shared/qt256.png"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
)

qt_internal_add_resource(triquadcube "triquadcube"
    PREFIX
        "/"
    FILES
        ${triquadcube_resource_files}
)
