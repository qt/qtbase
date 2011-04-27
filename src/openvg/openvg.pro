TARGET = QtOpenVG
QT += core \
    gui

DEFINES+=QT_BUILD_OPENVG_LIB

contains(QT_CONFIG, shivavg) {
    DEFINES += QVG_NO_DRAW_GLYPHS
    DEFINES += QVG_NO_RENDER_TO_MASK
    DEFINES += QVG_SCISSOR_CLIP
}

HEADERS += \
    qvg.h \
    qvg_p.h \
    qpaintengine_vg_p.h \
    qpixmapdata_vg_p.h \
    qpixmapfilter_vg_p.h \
    qvgcompositionhelper_p.h \
    qvgimagepool_p.h \
    qvgfontglyphcache_p.h
SOURCES += \
    qpaintengine_vg.cpp \
    qpixmapdata_vg.cpp \
    qpixmapfilter_vg.cpp \
    qvgimagepool.cpp

contains(QT_CONFIG, egl) {
    HEADERS += \
        qwindowsurface_vgegl_p.h \
        qwindowsurface_vg_p.h
    SOURCES += \
        qwindowsurface_vg.cpp \
        qwindowsurface_vgegl.cpp
}

symbian {
    DEFINES += QVG_RECREATE_ON_SIZE_CHANGE QVG_BUFFER_SCROLLING QVG_SCISSOR_CLIP
    SOURCES += \
        qvg_symbian.cpp

    contains(QT_CONFIG, freetype) {
        DEFINES += QT_NO_FONTCONFIG
        INCLUDEPATH += \
            ../3rdparty/freetype/src \
            ../3rdparty/freetype/include
    }
}

include(../qbase.pri)

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui
symbian:TARGET.UID3 = 0x2001E62F

!isEmpty(QMAKE_INCDIR_OPENVG): INCLUDEPATH += $$QMAKE_INCDIR_OPENVG
!isEmpty(QMAKE_LIBDIR_OPENVG): LIBS_PRIVATE += -L$$QMAKE_LIBDIR_OPENVG
!isEmpty(QMAKE_LIBS_OPENVG): LIBS_PRIVATE += $$QMAKE_LIBS_OPENVG

contains(QT_CONFIG, egl) {
    !isEmpty(QMAKE_INCDIR_EGL): INCLUDEPATH += $$QMAKE_INCDIR_EGL
    !isEmpty(QMAKE_LIBDIR_EGL): LIBS_PRIVATE += -L$$QMAKE_LIBDIR_EGL
    !isEmpty(QMAKE_LIBS_EGL): LIBS_PRIVATE += $$QMAKE_LIBS_EGL
}

contains(QT_CONFIG, openvg_on_opengl) {
    !isEmpty(QMAKE_INCDIR_OPENGL): INCLUDEPATH += $$QMAKE_INCDIR_OPENGL
    !isEmpty(QMAKE_LIBDIR_OPENGL): LIBS_PRIVATE += -L$$QMAKE_LIBDIR_OPENGL
    !isEmpty(QMAKE_LIBS_OPENGL): LIBS_PRIVATE += $$QMAKE_LIBS_OPENGL
}

INCLUDEPATH += ../3rdparty/harfbuzz/src
