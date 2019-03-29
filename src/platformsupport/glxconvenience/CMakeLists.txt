find_package(X11) # special case
find_package(OpenGL) # special case

#####################################################################
## GlxSupport Module:
#####################################################################

add_qt_module(GlxSupport
    STATIC
    SOURCES
        qglxconvenience.cpp qglxconvenience_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        OpenGL::GLX # special case
        Qt::CorePrivate
        Qt::GuiPrivate
        X11::X11 # special case
)

#### Keys ignored in scope 1:.:glxconvenience.pro:<NONE>:
# CONFIG = "static" "internal_module"
# MODULE = "glx_support"
# _LOADED = "qt_module"
