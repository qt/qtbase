# Generated from glxconvenience.pro.

qt_find_package(X11) # special case

#####################################################################
## GlxSupport Module:
#####################################################################

qt_add_module(GlxSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qglxconvenience.cpp qglxconvenience_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        X11::X11 # special case
)

#### Keys ignored in scope 1:.:.:glxconvenience.pro:<TRUE>:
# MODULE = "glx_support"
