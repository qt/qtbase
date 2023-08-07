# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## lance Binary:
#####################################################################

qt_internal_add_manual_test(lance
    SOURCES
        ../../baseline/shared/paintcommands.cpp ../../baseline/shared/paintcommands.h
        interactivewidget.cpp interactivewidget.h
        main.cpp
        widgets.h
    NO_PCH_SOURCES
        interactivewidget.cpp # undef QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        .
        ../../baseline/shared
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGLWidgets
        Qt::PrintSupport
        Qt::Widgets
)

# Resources:
set(icons_resource_files
    "enum.png"
    "tools.png"
)

qt_internal_add_resource(lance "icons"
    PREFIX
        "/icons"
    FILES
        ${icons_resource_files}
)
set(images_resource_files
    "../../baseline/painting/images/alpha.png"
    "../../baseline/painting/images/alpha2x2.png"
    "../../baseline/painting/images/bitmap.png"
    "../../baseline/painting/images/border.png"
    "../../baseline/painting/images/borderimage.png"
    "../../baseline/painting/images/dome_argb32.png"
    "../../baseline/painting/images/dome_indexed.png"
    "../../baseline/painting/images/dome_indexed_mask.png"
    "../../baseline/painting/images/dome_mono.png"
    "../../baseline/painting/images/dome_mono_128.png"
    "../../baseline/painting/images/dome_mono_palette.png"
    "../../baseline/painting/images/dome_rgb32.png"
    "../../baseline/painting/images/dot.png"
    "../../baseline/painting/images/face.png"
    "../../baseline/painting/images/gam030.png"
    "../../baseline/painting/images/gam045.png"
    "../../baseline/painting/images/gam056.png"
    "../../baseline/painting/images/gam100.png"
    "../../baseline/painting/images/gam200.png"
    "../../baseline/painting/images/image.png"
    "../../baseline/painting/images/mask.png"
    "../../baseline/painting/images/mask_100.png"
    "../../baseline/painting/images/masked.png"
    "../../baseline/painting/images/sign.png"
    "../../baseline/painting/images/solid.png"
    "../../baseline/painting/images/solid2x2.png"
    "../../baseline/painting/images/struct-image-01.jpg"
    "../../baseline/painting/images/struct-image-01.png"
    "../../baseline/painting/images/zebra.png"
)
qt_internal_add_resource(lance "images"
    PREFIX
        "/"
    BASE
        "../../baseline/painting"
    FILES
        ${images_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(lance CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
)
