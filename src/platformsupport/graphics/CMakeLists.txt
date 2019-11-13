# Generated from graphics.pro.

#####################################################################
## GraphicsSupport Module:
#####################################################################

qt_add_module(GraphicsSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qrasterbackingstore.cpp qrasterbackingstore_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:graphics.pro:<TRUE>:
# MODULE = "graphics_support"
