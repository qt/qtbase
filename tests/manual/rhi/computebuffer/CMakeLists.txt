# Generated from computebuffer.pro.

#####################################################################
## computebuffer Binary:
#####################################################################

add_qt_manual_test(computebuffer
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

add_qt_resource(computebuffer "computebuffer"
    PREFIX
        "/"
    FILES
        ${computebuffer_resource_files}
)


#### Keys ignored in scope 1:.:.:computebuffer.pro:<TRUE>:
# TEMPLATE = "app"
