# Generated from cubemap.pro.

#####################################################################
## cubemap Binary:
#####################################################################

add_qt_manual_test(cubemap
    GUI
    SOURCES
        cubemap.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(cubemap_resource_files
    "c.png"
    "cubemap.frag.qsb"
    "cubemap.vert.qsb"
)

add_qt_resource(cubemap "cubemap"
    PREFIX
        "/"
    FILES
        ${cubemap_resource_files}
)


#### Keys ignored in scope 1:.:.:cubemap.pro:<TRUE>:
# TEMPLATE = "app"
