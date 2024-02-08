# Generated from qtexturefilereader.pro.

#####################################################################
## tst_qtexturefilereader Test:
#####################################################################

qt_internal_add_test(tst_qtexturefilereader
    SOURCES
        tst_qtexturefilereader.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(qtexturefilereader_resource_files
    "texturefiles/car.ktx"
    "texturefiles/car_mips.ktx"
    "texturefiles/cubemap_float32_rgba.ktx"
    "texturefiles/cubemap_metadata.ktx"
    "texturefiles/invalid.ktx"
    "texturefiles/newlogo.astc"
    "texturefiles/newlogo_srgb.astc"
    "texturefiles/pattern.pkm"
)

qt_internal_add_resource(tst_qtexturefilereader "qtexturefilereader"
    PREFIX
        "/"
    FILES
        ${qtexturefilereader_resource_files}
)

