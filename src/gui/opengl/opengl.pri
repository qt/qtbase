# Qt gui library, opengl module

contains(QT_CONFIG, opengl):CONFIG += opengl
contains(QT_CONFIG, opengles2):CONFIG += opengles2
contains(QT_CONFIG, egl):CONFIG += egl

HEADERS += opengl/qopengl.h \
           opengl/qopengl_p.h \
           opengl/qopenglfunctions.h \
           opengl/qopenglframebufferobject.h  \
           opengl/qopenglframebufferobject_p.h  \
           opengl/qopenglpaintdevice_p.h \
           opengl/qopenglbuffer.h \
           opengl/qopenglshaderprogram.h \
           opengl/qopenglextensions_p.h \
           opengl/qopenglgradientcache_p.h \
           opengl/qopenglengineshadermanager_p.h \
           opengl/qopengl2pexvertexarray_p.h \
           opengl/qpaintengineex_opengl2_p.h \
           opengl/qopenglengineshadersource_p.h \
           opengl/qopenglcustomshaderstage_p.h \
           opengl/qtriangulatingstroker_p.h \
           opengl/qtriangulator_p.h \
           opengl/qrbtree_p.h \
           opengl/qtextureglyphcache_gl_p.h \
           opengl/qopenglshadercache_p.h \
           opengl/qopenglshadercache_meego_p.h \
           opengl/qopenglcolormap.h

SOURCES += opengl/qopengl.cpp \
           opengl/qopenglfunctions.cpp \
           opengl/qopenglframebufferobject.cpp \
           opengl/qopenglpaintdevice.cpp \
           opengl/qopenglbuffer.cpp \
           opengl/qopenglshaderprogram.cpp \
           opengl/qopenglgradientcache.cpp \
           opengl/qopenglengineshadermanager.cpp \
           opengl/qopengl2pexvertexarray.cpp \
           opengl/qpaintengineex_opengl2.cpp \
           opengl/qopenglcustomshaderstage.cpp \
           opengl/qtriangulatingstroker.cpp \
           opengl/qtriangulator.cpp \
           opengl/qtextureglyphcache_gl.cpp \
           opengl/qopenglcolormap.cpp

#INCLUDEPATH += ../3rdparty/harfbuzz/src
