# Generated from opengl.pro.

#####################################################################
## OpenGL Module:
#####################################################################

add_qt_module(OpenGL
    SOURCES
        gl2paintengineex/qgl2pexvertexarray.cpp gl2paintengineex/qgl2pexvertexarray_p.h
        gl2paintengineex/qglcustomshaderstage.cpp gl2paintengineex/qglcustomshaderstage_p.h
        gl2paintengineex/qglengineshadermanager.cpp gl2paintengineex/qglengineshadermanager_p.h
        gl2paintengineex/qglengineshadersource_p.h
        gl2paintengineex/qglgradientcache.cpp gl2paintengineex/qglgradientcache_p.h
        gl2paintengineex/qglshadercache_p.h
        gl2paintengineex/qpaintengineex_opengl2.cpp gl2paintengineex/qpaintengineex_opengl2_p.h
        gl2paintengineex/qtextureglyphcache_gl.cpp gl2paintengineex/qtextureglyphcache_gl_p.h
        qgl.cpp qgl.h qgl_p.h
        qglbuffer.cpp qglbuffer.h
        qglcolormap.cpp qglcolormap.h
        qglframebufferobject.cpp qglframebufferobject.h qglframebufferobject_p.h
        qglfunctions.cpp qglfunctions.h
        qglpaintdevice.cpp qglpaintdevice_p.h
        qglpixelbuffer.cpp qglpixelbuffer.h qglpixelbuffer_p.h
        qglshaderprogram.cpp qglshaderprogram.h
        qtopenglglobal.h
    DEFINES
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
        WrapOpenGL # special case
    PUBLIC_LIBRARIES # special case
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:opengl.pro:<NONE>:
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(OpenGL CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x63000000"
)

#### Keys ignored in scope 3:.:opengl.pro:solaris-cc_x_:
# QMAKE_CXXFLAGS_RELEASE = "--O2"

#### Keys ignored in scope 4:.:opengl.pro:QT_FEATURE_opengl:
# CONFIG = "opengl"

#### Keys ignored in scope 5:.:opengl.pro:QT_FEATURE_opengles2:
# CONFIG = "opengles2"

extend_target(OpenGL CONDITION QT_FEATURE_graphicseffect
    SOURCES
        qgraphicsshadereffect.cpp qgraphicsshadereffect_p.h
)

add_qt_docs(./doc/qtopengl.qdocconf)
