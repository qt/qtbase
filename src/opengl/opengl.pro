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
    qopenglversionfunctions.h \
    qopenglversionfunctions_p.h \
    qopenglversionfunctionsfactory.h \
    qopenglversionprofile.h \
    qopenglvertexarrayobject.h \
    qopenglwindow.h \
    qtopenglglobal.h \
    qplatformbackingstoreopenglsupport.h

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
    qopenglversionfunctions.cpp \
    qopenglversionfunctionsfactory.cpp \
    qopenglversionprofile.cpp \
    qopenglvertexarrayobject.cpp \
    qopenglwindow.cpp \
    qopengldebug.cpp \
    qplatformbackingstoreopenglsupport.cpp

!qtConfig(opengles2) {
    HEADERS += \
        qopenglfunctions_1_0.h \
        qopenglfunctions_1_1.h \
        qopenglfunctions_1_2.h \
        qopenglfunctions_1_3.h \
        qopenglfunctions_1_4.h \
        qopenglfunctions_1_5.h \
        qopenglfunctions_2_0.h \
        qopenglfunctions_2_1.h \
        qopenglfunctions_3_0.h \
        qopenglfunctions_3_1.h \
        qopenglfunctions_3_2_core.h \
        qopenglfunctions_3_3_core.h \
        qopenglfunctions_4_0_core.h \
        qopenglfunctions_4_1_core.h \
        qopenglfunctions_4_2_core.h \
        qopenglfunctions_4_3_core.h \
        qopenglfunctions_4_4_core.h \
        qopenglfunctions_4_5_core.h \
        qopenglfunctions_3_2_compatibility.h \
        qopenglfunctions_3_3_compatibility.h \
        qopenglfunctions_4_0_compatibility.h \
        qopenglfunctions_4_1_compatibility.h \
        qopenglfunctions_4_2_compatibility.h \
        qopenglfunctions_4_3_compatibility.h \
        qopenglfunctions_4_4_compatibility.h \
        qopenglfunctions_4_5_compatibility.h \
        qopenglqueryhelper_p.h \
        qopengltimerquery.h

    SOURCES += \
        qopenglfunctions_1_0.cpp \
        qopenglfunctions_1_1.cpp \
        qopenglfunctions_1_2.cpp \
        qopenglfunctions_1_3.cpp \
        qopenglfunctions_1_4.cpp \
        qopenglfunctions_1_5.cpp \
        qopenglfunctions_2_0.cpp \
        qopenglfunctions_2_1.cpp \
        qopenglfunctions_3_0.cpp \
        qopenglfunctions_3_1.cpp \
        qopenglfunctions_3_2_core.cpp \
        qopenglfunctions_3_3_core.cpp \
        qopenglfunctions_4_0_core.cpp \
        qopenglfunctions_4_1_core.cpp \
        qopenglfunctions_4_2_core.cpp \
        qopenglfunctions_4_3_core.cpp \
        qopenglfunctions_4_4_core.cpp \
        qopenglfunctions_4_5_core.cpp \
        qopenglfunctions_3_2_compatibility.cpp \
        qopenglfunctions_3_3_compatibility.cpp \
        qopenglfunctions_4_0_compatibility.cpp \
        qopenglfunctions_4_1_compatibility.cpp \
        qopenglfunctions_4_2_compatibility.cpp \
        qopenglfunctions_4_3_compatibility.cpp \
        qopenglfunctions_4_4_compatibility.cpp \
        qopenglfunctions_4_5_compatibility.cpp \
        qopengltimerquery.cpp
}

qtConfig(opengles2) {
    HEADERS += qopenglfunctions_es2.h
    SOURCES += qopenglfunctions_es2.cpp
}

qtConfig(vulkan) {
    CONFIG += generated_privates

    HEADERS += qvkconvenience_p.h
    SOURCES += qvkconvenience.cpp

    # Applications must inherit the Vulkan header include path.
    QMAKE_USE += vulkan/nolink
}

qtConfig(egl) {
    SOURCES += \
        qopenglcompositorbackingstore.cpp \
        qopenglcompositor.cpp

    HEADERS += \
        qopenglcompositorbackingstore_p.h \
        qopenglcompositor_p.h
}

load(qt_module)
