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
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:platformcompositor.pro:<TRUE>:
# MODULE = "platformcompositor_support"
