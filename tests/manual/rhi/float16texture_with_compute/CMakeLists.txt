# Generated from float16texture_with_compute.pro.

#####################################################################
## float16texture_with_compute Binary:
#####################################################################

qt_internal_add_manual_test(float16texture_with_compute
    GUI
    SOURCES
        float16texture_with_compute.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/qt256.png"
    PROPERTIES QT_RESOURCE_ALIAS "qt256.png"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(float16texture_with_compute_resource_files
    "../shared/qt256.png"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
    "load.comp.qsb"
    "prefilter.comp.qsb"
)

qt_internal_add_resource(float16texture_with_compute "float16texture_with_compute"
    PREFIX
        "/"
    FILES
        ${float16texture_with_compute_resource_files}
)


#### Keys ignored in scope 1:.:.:float16texture_with_compute.pro:<TRUE>:
# TEMPLATE = "app"
