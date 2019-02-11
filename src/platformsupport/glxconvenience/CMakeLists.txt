# Generated from glxconvenience.pro.

find_package(X11)
find_package(OpenGL)

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
        Qt::CorePrivate
        Qt::GuiPrivate
        X11::X11
        OpenGL::GLX
    # CONFIG = "static" "internal_module"
    # MODULE = "glx_support"
    # _LOADED = "qt_module"
)
