# Generated from instancing.pro.

#####################################################################
## instancing Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_instancing
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

qt_internal_add_resource(tst_manual_instancing "instancing"
    PREFIX
        "/"
    FILES
        ${instancing_resource_files}
)


#### Keys ignored in scope 1:.:.:instancing.pro:<TRUE>:
# TEMPLATE = "app"
