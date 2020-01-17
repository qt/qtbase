# Generated from opengl.pro.

#####################################################################
## OpenGL Module:
#####################################################################

qt_add_module(OpenGL
    SOURCES
        qopengl2pexvertexarray.cpp qopengl2pexvertexarray_p.h
        qopenglcustomshaderstage.cpp qopenglcustomshaderstage_p.h
        qopengldebug.cpp qopengldebug.h
        qopenglengineshadermanager.cpp qopenglengineshadermanager_p.h
        qopenglengineshadersource_p.h
        qopenglframebufferobject.cpp qopenglframebufferobject.h qopenglframebufferobject_p.h
        qopenglgradientcache.cpp qopenglgradientcache_p.h
        qopenglpaintdevice.cpp qopenglpaintdevice.h qopenglpaintdevice_p.h
        qopenglpaintengine.cpp qopenglpaintengine_p.h
        qopenglpixeltransferoptions.cpp qopenglpixeltransferoptions.h
        qopenglshadercache_p.h
        qopengltexture.cpp qopengltexture.h qopengltexture_p.h
        qopengltextureblitter.cpp qopengltextureblitter.h
        qopengltexturecache.cpp qopengltexturecache_p.h
        qopengltextureglyphcache.cpp qopengltextureglyphcache_p.h
        qopengltexturehelper.cpp qopengltexturehelper_p.h
        qopengltextureuploader.cpp qopengltextureuploader_p.h
        qopenglwindow.cpp qopenglwindow.h
        qtopenglglobal.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_extend_target(OpenGL CONDITION QT_FEATURE_widgets
    SOURCES
        qopenglwidget.cpp qopenglwidget.h
    LIBRARIES
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Widgets
    PRIVATE_MODULE_INTERFACE
        Qt::WidgetsPrivate
)

qt_extend_target(OpenGL CONDITION NOT QT_FEATURE_opengles2
    SOURCES
        qopenglqueryhelper_p.h
        qopengltimerquery.cpp qopengltimerquery.h
)
qt_add_docs(OpenGL
    doc/qtopengl.qdocconf
)

