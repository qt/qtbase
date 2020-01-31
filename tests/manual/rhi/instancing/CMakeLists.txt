# Generated from instancing.pro.

#####################################################################
## instancing Binary:
#####################################################################

add_qt_manual_test(instancing
    GUI
    SOURCES
        instancing.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set(instancing_resource_files
    "inst.frag.qsb"
    "inst.vert.qsb"
)

add_qt_resource(instancing "instancing"
    PREFIX
        "/"
    FILES
        ${instancing_resource_files}
)


#### Keys ignored in scope 1:.:.:instancing.pro:<TRUE>:
# TEMPLATE = "app"
