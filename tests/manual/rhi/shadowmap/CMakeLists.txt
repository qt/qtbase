# Generated from shadowmap.pro.

#####################################################################
## shadowmap Binary:
#####################################################################

add_qt_manual_test(shadowmap
    GUI
    SOURCES
        shadowmap.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set(shadowmap_resource_files
    "main.frag.qsb"
    "main.vert.qsb"
    "shadowmap.frag.qsb"
    "shadowmap.vert.qsb"
)

add_qt_resource(shadowmap "shadowmap"
    PREFIX
        "/"
    FILES
        ${shadowmap_resource_files}
)


#### Keys ignored in scope 1:.:.:shadowmap.pro:<TRUE>:
# TEMPLATE = "app"
