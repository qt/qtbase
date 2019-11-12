# Generated from msaarenderbuffer.pro.

#####################################################################
## msaarenderbuffer Binary:
#####################################################################

add_qt_manual_test(msaarenderbuffer
    GUI
    SOURCES
        msaarenderbuffer.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set_source_files_properties("../shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(msaarenderbuffer_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
)

add_qt_resource(msaarenderbuffer "msaarenderbuffer"
    PREFIX
        "/"
    FILES
        ${msaarenderbuffer_resource_files}
)


#### Keys ignored in scope 1:.:.:msaarenderbuffer.pro:<TRUE>:
# TEMPLATE = "app"
