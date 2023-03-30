# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## OpenGL Module:
#####################################################################

qt_internal_add_module(OpenGL
    SOURCES
        qopengl2pexvertexarray.cpp qopengl2pexvertexarray_p.h
        qopenglbuffer.cpp qopenglbuffer.h
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
        qopenglshaderprogram.cpp qopenglshaderprogram.h
        qopengltexture.cpp qopengltexture.h qopengltexture_p.h
        qopengltextureblitter.cpp qopengltextureblitter.h
        qopengltexturecache.cpp qopengltexturecache_p.h
        qopengltextureglyphcache.cpp qopengltextureglyphcache_p.h
        qopengltexturehelper.cpp qopengltexturehelper_p.h
        qopengltextureuploader.cpp qopengltextureuploader_p.h
        qopengltimerquery.h
        qopenglversionfunctions.cpp qopenglversionfunctions.h qopenglversionfunctions_p.h
        qopenglversionfunctionsfactory.cpp qopenglversionfunctionsfactory.h
        qopenglversionprofile.cpp qopenglversionprofile.h
        qopenglvertexarrayobject.cpp qopenglvertexarrayobject.h qopenglvertexarrayobject_p.h
        qopenglwindow.cpp qopenglwindow.h
        qtopenglglobal.h
    NO_UNITY_BUILD_SOURCES
        qopengltextureblitter.cpp       # qDebug()<< ambiguities
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
    GENERATE_CPP_EXPORTS
)

## Scopes:
#####################################################################

qt_internal_extend_target(OpenGL CONDITION APPLE
    NO_UNITY_BUILD_SOURCES
        qopenglversionfunctionsfactory.cpp
)

qt_internal_extend_target(OpenGL CONDITION QT_FEATURE_opengles2
    SOURCES
        qopenglfunctions_es2.cpp qopenglfunctions_es2.h
)

qt_internal_extend_target(OpenGL CONDITION NOT QT_FEATURE_opengles2
    SOURCES
        qopenglfunctions_1_0.cpp qopenglfunctions_1_0.h
        qopenglfunctions_1_1.cpp qopenglfunctions_1_1.h
        qopenglfunctions_1_2.cpp qopenglfunctions_1_2.h
        qopenglfunctions_1_3.cpp qopenglfunctions_1_3.h
        qopenglfunctions_1_4.cpp qopenglfunctions_1_4.h
        qopenglfunctions_1_5.cpp qopenglfunctions_1_5.h
        qopenglfunctions_2_0.cpp qopenglfunctions_2_0.h
        qopenglfunctions_2_1.cpp qopenglfunctions_2_1.h
        qopenglfunctions_3_0.cpp qopenglfunctions_3_0.h
        qopenglfunctions_3_1.cpp qopenglfunctions_3_1.h
        qopenglfunctions_3_2_compatibility.cpp qopenglfunctions_3_2_compatibility.h
        qopenglfunctions_3_2_core.cpp qopenglfunctions_3_2_core.h
        qopenglfunctions_3_3_compatibility.cpp qopenglfunctions_3_3_compatibility.h
        qopenglfunctions_3_3_core.cpp qopenglfunctions_3_3_core.h
        qopenglfunctions_4_0_compatibility.cpp qopenglfunctions_4_0_compatibility.h
        qopenglfunctions_4_0_core.cpp qopenglfunctions_4_0_core.h
        qopenglfunctions_4_1_compatibility.cpp qopenglfunctions_4_1_compatibility.h
        qopenglfunctions_4_1_core.cpp qopenglfunctions_4_1_core.h
        qopenglfunctions_4_2_compatibility.cpp qopenglfunctions_4_2_compatibility.h
        qopenglfunctions_4_2_core.cpp qopenglfunctions_4_2_core.h
        qopenglfunctions_4_3_compatibility.cpp qopenglfunctions_4_3_compatibility.h
        qopenglfunctions_4_3_core.cpp qopenglfunctions_4_3_core.h
        qopenglfunctions_4_4_compatibility.cpp qopenglfunctions_4_4_compatibility.h
        qopenglfunctions_4_4_core.cpp qopenglfunctions_4_4_core.h
        qopenglfunctions_4_5_compatibility.cpp qopenglfunctions_4_5_compatibility.h
        qopenglfunctions_4_5_core.cpp qopenglfunctions_4_5_core.h
        qopenglqueryhelper_p.h
        qopengltimerquery.cpp
)

qt_internal_extend_target(OpenGL CONDITION QT_FEATURE_vulkan
    SOURCES
        qvkconvenience.cpp qvkconvenience_p.h
)
if(QT_FEATURE_vulkan)
    qt_internal_add_target_include_dirs_and_optionally_propagate(
        OpenGL WrapVulkanHeaders::WrapVulkanHeaders)
endif()

qt_internal_extend_target(OpenGL CONDITION QT_FEATURE_egl
    SOURCES
        qopenglcompositor.cpp qopenglcompositor_p.h
        qopenglcompositorbackingstore.cpp qopenglcompositorbackingstore_p.h
)

qt_internal_generate_tracepoints(OpenGL opengl
    SOURCES
        qopenglframebufferobject.cpp
        qopenglpaintengine.cpp
        qopengltexturecache.cpp
)
qt_internal_add_docs(OpenGL
    doc/qtopengl.qdocconf
)
