# Generated from offscreen.pro.

#####################################################################
## offscreen Binary:
#####################################################################

qt_internal_add_manual_test(offscreen
    SOURCES
        offscreen.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set(offscreen_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
)

qt_internal_add_resource(offscreen "offscreen"
    PREFIX
        "/"
    FILES
        ${offscreen_resource_files}
)


#### Keys ignored in scope 1:.:.:offscreen.pro:<TRUE>:
# TEMPLATE = "app"
