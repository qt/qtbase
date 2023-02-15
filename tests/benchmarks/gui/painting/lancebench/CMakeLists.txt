# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_lancebench Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_lancebench
    SOURCES
        ../../../../baseline/shared/paintcommands.cpp ../../../../baseline/shared/paintcommands.h
        tst_lancebench.cpp
    INCLUDE_DIRECTORIES
        ../../../../baseline/shared
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
)

# Resources:
set(images_resource_files
    "images/alpha.png"
    "images/alpha2x2.png"
    "images/bitmap.png"
    "images/border.png"
    "images/borderimage.png"
    "images/dome_argb32.png"
    "images/dome_indexed.png"
    "images/dome_indexed_mask.png"
    "images/dome_mono.png"
    "images/dome_mono_128.png"
    "images/dome_mono_palette.png"
    "images/dome_rgb32.png"
    "images/dot.png"
    "images/face.png"
    "images/gam030.png"
    "images/gam045.png"
    "images/gam056.png"
    "images/gam100.png"
    "images/gam200.png"
    "images/image.png"
    "images/mask.png"
    "images/mask_100.png"
    "images/masked.png"
    "images/sign.png"
    "images/solid.png"
    "images/solid2x2.png"
    "images/struct-image-01.jpg"
    "images/struct-image-01.png"
    "images/zebra.png"
)

list(TRANSFORM images_resource_files PREPEND "../../../../baseline/painting/")

qt_internal_add_resource(tst_bench_lancebench "images"
    PREFIX
        "/"
    BASE
        "../../../../baseline/painting"
    FILES
        ${images_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_lancebench CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGL
)
