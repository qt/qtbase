# Generated from instancing.pro.

#####################################################################
## instancing Binary:
#####################################################################

add_qt_manual_test(instancing
    GUI
    SOURCES
        instancing.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
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
