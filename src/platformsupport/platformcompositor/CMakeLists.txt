# Generated from platformcompositor.pro.

#####################################################################
## PlatformCompositorSupport Module:
#####################################################################

add_qt_module(PlatformCompositorSupport
    STATIC
    SOURCES
        qopenglcompositor.cpp qopenglcompositor_p.h
        qopenglcompositorbackingstore.cpp qopenglcompositorbackingstore_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:platformcompositor.pro:<NONE>:
# CONFIG = "static" "internal_module"
# MODULE = "platformcompositor_support"
# _LOADED = "qt_module"
