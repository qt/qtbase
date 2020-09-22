# Generated from computebuffer.pro.

#####################################################################
## computebuffer Binary:
#####################################################################

qt_internal_add_manual_test(computebuffer
    GUI
    SOURCES
        computebuffer.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(computebuffer_resource_files
    "buffer.comp.qsb"
    "main.frag.qsb"
    "main.vert.qsb"
)

qt_internal_add_resource(computebuffer "computebuffer"
    PREFIX
        "/"
    FILES
        ${computebuffer_resource_files}
)


#### Keys ignored in scope 1:.:.:computebuffer.pro:<TRUE>:
# TEMPLATE = "app"
