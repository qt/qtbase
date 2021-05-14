# Generated from lance.pro.

#####################################################################
## lance Binary:
#####################################################################

qt_internal_add_manual_test(lance
    SOURCES
        ../../auto/other/lancelot/paintcommands.cpp ../../auto/other/lancelot/paintcommands.h
        interactivewidget.cpp interactivewidget.h
        main.cpp
        widgets.h
    INCLUDE_DIRECTORIES
        .
        ../../auto/other/lancelot
    PUBLIC_LIBRARIES
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
    "../../auto/other/lancelot/images/alpha.png"
    "../../auto/other/lancelot/images/alpha2x2.png"
    "../../auto/other/lancelot/images/bitmap.png"
    "../../auto/other/lancelot/images/border.png"
    "../../auto/other/lancelot/images/borderimage.png"
    "../../auto/other/lancelot/images/dome_argb32.png"
    "../../auto/other/lancelot/images/dome_indexed.png"
    "../../auto/other/lancelot/images/dome_indexed_mask.png"
    "../../auto/other/lancelot/images/dome_mono.png"
    "../../auto/other/lancelot/images/dome_mono_128.png"
    "../../auto/other/lancelot/images/dome_mono_palette.png"
    "../../auto/other/lancelot/images/dome_rgb32.png"
    "../../auto/other/lancelot/images/dot.png"
    "../../auto/other/lancelot/images/face.png"
    "../../auto/other/lancelot/images/gam030.png"
    "../../auto/other/lancelot/images/gam045.png"
    "../../auto/other/lancelot/images/gam056.png"
    "../../auto/other/lancelot/images/gam100.png"
    "../../auto/other/lancelot/images/gam200.png"
    "../../auto/other/lancelot/images/image.png"
    "../../auto/other/lancelot/images/mask.png"
    "../../auto/other/lancelot/images/mask_100.png"
    "../../auto/other/lancelot/images/masked.png"
    "../../auto/other/lancelot/images/sign.png"
    "../../auto/other/lancelot/images/solid.png"
    "../../auto/other/lancelot/images/solid2x2.png"
    "../../auto/other/lancelot/images/struct-image-01.jpg"
    "../../auto/other/lancelot/images/struct-image-01.png"
    "../../auto/other/lancelot/images/zebra.png"
)
qt_internal_add_resource(lance "images"
    PREFIX
        "/"
    BASE
        "../../auto/other/lancelot"
    FILES
        ${images_resource_files}
)


#### Keys ignored in scope 1:.:.:lance.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(lance CONDITION TARGET Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
