# Generated from texuploads.pro.

#####################################################################
## texuploads Binary:
#####################################################################

qt_internal_add_manual_test(texuploads
    GUI
    SOURCES
        texuploads.cpp
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
set(texuploads_resource_files
    "../shared/qt256.png"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
)

qt_internal_add_resource(texuploads "texuploads"
    PREFIX
        "/"
    FILES
        ${texuploads_resource_files}
)


#### Keys ignored in scope 1:.:.:texuploads.pro:<TRUE>:
# TEMPLATE = "app"
