# Generated from lancebench.pro.

#####################################################################
## tst_bench_lancebench Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_lancebench
    SOURCES
        ../../../../auto/other/lancelot/paintcommands.cpp ../../../../auto/other/lancelot/paintcommands.h
        tst_lancebench.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
)

# Resources:
set_source_files_properties("../../../../auto/other/lancelot/images/alpha.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/alpha.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/alpha2x2.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/alpha2x2.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/bitmap.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/bitmap.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/border.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/border.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/borderimage.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/borderimage.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_argb32.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_argb32.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_indexed.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_indexed.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_indexed_mask.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_indexed_mask.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_mono.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_mono.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_mono_128.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_mono_128.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_mono_palette.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_mono_palette.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dome_rgb32.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dome_rgb32.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/dot.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/dot.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/face.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/face.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/gam030.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/gam030.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/gam045.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/gam045.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/gam056.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/gam056.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/gam100.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/gam100.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/gam200.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/gam200.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/image.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/image.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/mask.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/mask.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/mask_100.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/mask_100.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/masked.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/masked.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/sign.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/sign.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/solid.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/solid.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/solid2x2.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/solid2x2.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/struct-image-01.jpg"
    PROPERTIES QT_RESOURCE_ALIAS "images/struct-image-01.jpg"
)
set_source_files_properties("../../../../auto/other/lancelot/images/struct-image-01.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/struct-image-01.png"
)
set_source_files_properties("../../../../auto/other/lancelot/images/zebra.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/zebra.png"
)
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

qt_internal_add_resource(tst_bench_lancebench "images"
    PREFIX
        "/"
    BASE
        "../../../../auto/other/lancelot"
    FILES
        ${images_resource_files}
)


#### Keys ignored in scope 1:.:.:lancebench.pro:<TRUE>:
# TEMPLATE = "app"
# TESTDATA = "../../../../auto/other/lancelot/scripts/*"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_lancebench CONDITION QT_FEATURE_opengl
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
