# Generated from platformcompositor.pro.

#####################################################################
## PlatformCompositorSupport Module:
#####################################################################

qt_add_module(PlatformCompositorSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qopenglcompositor.cpp qopenglcompositor_p.h
        qopenglcompositorbackingstore.cpp qopenglcompositorbackingstore_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:platformcompositor.pro:<TRUE>:
# MODULE = "platformcompositor_support"
