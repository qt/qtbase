# Generated from qtexturefilereader.pro.

#####################################################################
## tst_qtexturefilereader Test:
#####################################################################

add_qt_test(tst_qtexturefilereader
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
    "texturefiles/newlogo.astc"
    "texturefiles/newlogo_srgb.astc"
    "texturefiles/pattern.pkm"
)

add_qt_resource(tst_qtexturefilereader "qtexturefilereader"
    PREFIX
        "/"
    FILES
        ${qtexturefilereader_resource_files}
)

