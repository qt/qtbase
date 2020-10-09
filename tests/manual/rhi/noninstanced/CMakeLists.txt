# Generated from noninstanced.pro.

#####################################################################
## noninstanced Binary:
#####################################################################

qt_internal_add_manual_test(noninstanced
    GUI
    SOURCES
        noninstanced.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(noninstanced_resource_files
    "material.frag.qsb"
    "material.vert.qsb"
)

qt_internal_add_resource(noninstanced "noninstanced"
    PREFIX
        "/"
    FILES
        ${noninstanced_resource_files}
)


#### Keys ignored in scope 1:.:.:noninstanced.pro:<TRUE>:
# TEMPLATE = "app"
