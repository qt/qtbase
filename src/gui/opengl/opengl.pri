# Qt gui library, opengl module

contains(QT_CONFIG, opengl):CONFIG += opengl
contains(QT_CONFIG, opengles2):CONFIG += opengles2
contains(QT_CONFIG, egl):CONFIG += egl

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles2) {

    HEADERS += opengl/qopengl.h \
               opengl/qopengl_p.h \
               opengl/qopenglfunctions.h \
               opengl/qopenglframebufferobject.h  \
               opengl/qopenglframebufferobject_p.h  \
               opengl/qopenglpaintdevice.h \
               opengl/qopenglbuffer.h \
               opengl/qopenglshaderprogram.h \
               opengl/qopenglextensions_p.h \
               opengl/qopenglgradientcache_p.h \
               opengl/qopengltexturecache_p.h \
               opengl/qopenglengineshadermanager_p.h \
               opengl/qopengl2pexvertexarray_p.h \
               opengl/qopenglpaintengine_p.h \
               opengl/qopenglengineshadersource_p.h \
               opengl/qopenglcustomshaderstage_p.h \
               opengl/qtriangulatingstroker_p.h \
               opengl/qopengltextureglyphcache_p.h \
               opengl/qopenglshadercache_p.h \
               opengl/qopenglshadercache_meego_p.h \
               opengl/qtriangulator_p.h \
               opengl/qrbtree_p.h

    SOURCES += opengl/qopengl.cpp \
               opengl/qopenglfunctions.cpp \
               opengl/qopenglframebufferobject.cpp \
               opengl/qopenglpaintdevice.cpp \
               opengl/qopenglbuffer.cpp \
               opengl/qopenglshaderprogram.cpp \
               opengl/qopenglgradientcache.cpp \
               opengl/qopengltexturecache.cpp \
               opengl/qopenglengineshadermanager.cpp \
               opengl/qopengl2pexvertexarray.cpp \
               opengl/qopenglpaintengine.cpp \
               opengl/qopenglcustomshaderstage.cpp \
               opengl/qtriangulatingstroker.cpp \
               opengl/qopengltextureglyphcache.cpp \
               opengl/qtriangulator.cpp

}
