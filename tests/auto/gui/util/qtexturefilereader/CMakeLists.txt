# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtexturefilereader Test:
#####################################################################

# Resources:
set(qtexturefilereader_resource_files
    "texturefiles/car.ktx"
    "texturefiles/car_mips.ktx"
    "texturefiles/cubemap_float32_rgba.ktx"
    "texturefiles/cubemap_metadata.ktx"
    "texturefiles/newlogo.astc"
    "texturefiles/newlogo_srgb.astc"
    "texturefiles/pattern.pkm"
)

qt_internal_add_test(tst_qtexturefilereader
    SOURCES
        tst_qtexturefilereader.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qtexturefilereader_resource_files}
    BUILTIN_TESTDATA
)

