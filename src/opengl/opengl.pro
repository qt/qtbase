TARGET     = QtOpenGL
QT         = core-private gui-private

DEFINES   += QT_NO_USING_NAMESPACE QT_NO_FOREACH

QMAKE_DOCS = $$PWD/doc/qtopengl.qdocconf

qtConfig(opengl): CONFIG += opengl
qtConfig(opengles2): CONFIG += opengles2

HEADERS += \
    qopengl2pexvertexarray_p.h \
    qopenglbuffer.h \
    qopenglcustomshaderstage_p.h \
    qopengldebug.h \
    qopenglengineshadermanager_p.h \
    qopenglengineshadersource_p.h \
    qopenglframebufferobject.h \
    qopenglframebufferobject_p.h \
    qopenglgradientcache_p.h \
    qopenglpaintdevice.h \
    qopenglpaintdevice_p.h \
    qopenglpaintengine_p.h \
    qopenglpixeltransferoptions.h \
    qopenglshadercache_p.h \
    qopenglshaderprogram.h \
    qopengltexture.h \
    qopengltexture_p.h \
    qopengltexturehelper_p.h \
    qopengltextureblitter.h \
    qopengltexturecache_p.h \
    qopengltextureglyphcache_p.h \
    qopengltextureuploader_p.h \
    qopenglvertexarrayobject.h \
    qopenglwindow.h \
    qtopenglglobal.h

SOURCES += \
    qopengl2pexvertexarray.cpp \
    qopenglbuffer.cpp \
    qopenglcustomshaderstage.cpp \
    qopenglengineshadermanager.cpp \
    qopenglframebufferobject.cpp \
    qopenglgradientcache.cpp \
    qopenglpaintdevice.cpp \
    qopenglpaintengine.cpp \
    qopenglpixeltransferoptions.cpp \
    qopenglshaderprogram.cpp \
    qopengltexture.cpp \
    qopengltexturehelper.cpp \
    qopengltextureblitter.cpp \
    qopengltexturecache.cpp \
    qopengltextureglyphcache.cpp \
    qopengltextureuploader.cpp \
    qopenglvertexarrayobject.cpp \
    qopenglwindow.cpp \
    qopengldebug.cpp

!qtConfig(opengles2) {
    HEADERS += \
        qopenglqueryhelper_p.h \
        qopengltimerquery.h

    SOURCES += qopengltimerquery.cpp
}

load(qt_module)
