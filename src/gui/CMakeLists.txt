# Generated from gui.pro.

set(OpenGL_GL_PREFERENCE GLVND) # special case:

#####################################################################
## Gui Module:
#####################################################################

# special case begin
if (QT_FEATURE_gui)
    if (WINRT)
        set(_default_platform "winrt")
    elseif(WIN32)
        set(_default_platform "windows")
    elseif(ANDROID)
        set(_default_platform "android")
    elseif(APPLE_OSX)
        set(_default_platform "cocoa")
    elseif(APPLE_TVOS OR APPLE_IOS)
        set(_default_platform "ios")
    elseif(APPLE_WATCHOS)
        set(_default_platform "minimal")
    elseif(QNX)
        set(_default_platform "qnx")
    elseif(INTEGRITY)
        set(_default_platform "integrityfb")
    elseif(HAIKU)
        set(_default_platform "haiku")
    elseif(WASM)
        set(_default_platform "webassembly")
    else()
        set(_default_platform "xcb")
    endif()

    set(QT_QPA_DEFAULT_PLATFORM "${_default_platform}" CACHE STRING "QPA default platform")
endif()
# special case end

add_qt_module(Gui
    FEATURE_DEPENDENCIES  # special case:
        Qt::Network # special case:
    SOURCES
        image/qbitmap.cpp image/qbitmap.h
        image/qbmphandler.cpp image/qbmphandler_p.h
        image/qicon.cpp image/qicon.h image/qicon_p.h
        image/qiconengine.cpp image/qiconengine.h
        image/qiconengineplugin.cpp image/qiconengineplugin.h
        image/qiconloader.cpp image/qiconloader_p.h
        image/qimage.cpp image/qimage.h image/qimage_p.h
        image/qimage_compat.cpp
        image/qimage_conversions.cpp
        image/qimageiohandler.cpp image/qimageiohandler.h
        image/qimagepixmapcleanuphooks.cpp image/qimagepixmapcleanuphooks_p.h
        image/qimagereader.cpp image/qimagereader.h
        image/qimagereaderwriterhelpers.cpp image/qimagereaderwriterhelpers_p.h
        image/qimagewriter.cpp image/qimagewriter.h
        image/qpaintengine_pic.cpp image/qpaintengine_pic_p.h
        image/qpicture.cpp image/qpicture.h image/qpicture_p.h
        image/qpictureformatplugin.cpp image/qpictureformatplugin.h
        image/qpixmap.cpp image/qpixmap.h
        image/qpixmap_blitter.cpp image/qpixmap_blitter_p.h
        image/qpixmap_raster.cpp image/qpixmap_raster_p.h
        image/qpixmapcache.cpp image/qpixmapcache.h image/qpixmapcache_p.h
        image/qplatformpixmap.cpp image/qplatformpixmap.h
        image/qppmhandler.cpp image/qppmhandler_p.h
        image/qxbmhandler.cpp image/qxbmhandler_p.h
        image/qxpmhandler.cpp image/qxpmhandler_p.h
        # itemmodels/qstandarditemmodel.cpp itemmodels/qstandarditemmodel.h itemmodels/qstandarditemmodel_p.h # special case
        kernel/qclipboard.cpp kernel/qclipboard.h
        kernel/qcursor.cpp kernel/qcursor.h kernel/qcursor_p.h
        kernel/qevent.cpp kernel/qevent.h kernel/qevent_p.h
        kernel/qgenericplugin.cpp kernel/qgenericplugin.h
        kernel/qgenericpluginfactory.cpp kernel/qgenericpluginfactory.h
        kernel/qguiapplication.cpp kernel/qguiapplication.h kernel/qguiapplication_p.h
        kernel/qguivariant.cpp
        kernel/qhighdpiscaling.cpp kernel/qhighdpiscaling_p.h
        kernel/qinputdevicemanager.cpp kernel/qinputdevicemanager_p.h
        kernel/qinputdevicemanager_p_p.h
        kernel/qinputmethod.cpp kernel/qinputmethod.h kernel/qinputmethod_p.h
        kernel/qinternalmimedata.cpp kernel/qinternalmimedata_p.h
        kernel/qkeymapper.cpp kernel/qkeymapper_p.h
        kernel/qkeysequence.cpp kernel/qkeysequence.h kernel/qkeysequence_p.h
        kernel/qoffscreensurface.cpp kernel/qoffscreensurface.h
        kernel/qpaintdevicewindow.cpp kernel/qpaintdevicewindow.h kernel/qpaintdevicewindow_p.h
        kernel/qpalette.cpp kernel/qpalette.h
        kernel/qpixelformat.cpp kernel/qpixelformat.h
        kernel/qplatformclipboard.cpp kernel/qplatformclipboard.h
        kernel/qplatformcursor.cpp kernel/qplatformcursor.h
        kernel/qplatformdialoghelper.cpp kernel/qplatformdialoghelper.h
        kernel/qplatformgraphicsbuffer.cpp kernel/qplatformgraphicsbuffer.h
        kernel/qplatformgraphicsbufferhelper.cpp kernel/qplatformgraphicsbufferhelper.h
        kernel/qplatforminputcontext.cpp kernel/qplatforminputcontext.h kernel/qplatforminputcontext_p.h
        kernel/qplatforminputcontextfactory.cpp kernel/qplatforminputcontextfactory_p.h
        kernel/qplatforminputcontextplugin.cpp kernel/qplatforminputcontextplugin_p.h
        kernel/qplatformintegration.cpp kernel/qplatformintegration.h
        kernel/qplatformintegrationfactory.cpp kernel/qplatformintegrationfactory_p.h
        kernel/qplatformintegrationplugin.cpp kernel/qplatformintegrationplugin.h
        kernel/qplatformmenu.cpp kernel/qplatformmenu.h
        kernel/qplatformnativeinterface.cpp kernel/qplatformnativeinterface.h
        kernel/qplatformoffscreensurface.cpp kernel/qplatformoffscreensurface.h
        kernel/qplatformscreen.cpp kernel/qplatformscreen.h kernel/qplatformscreen_p.h
        kernel/qplatformservices.cpp kernel/qplatformservices.h
        kernel/qplatformsessionmanager.cpp kernel/qplatformsessionmanager.h
        kernel/qplatformsharedgraphicscache.cpp kernel/qplatformsharedgraphicscache.h
        kernel/qplatformsurface.cpp kernel/qplatformsurface.h
        kernel/qplatformsystemtrayicon.cpp kernel/qplatformsystemtrayicon.h
        kernel/qplatformtheme.cpp kernel/qplatformtheme.h kernel/qplatformtheme_p.h
        kernel/qplatformthemefactory.cpp kernel/qplatformthemefactory_p.h
        kernel/qplatformthemeplugin.cpp kernel/qplatformthemeplugin.h
        kernel/qplatformwindow.cpp kernel/qplatformwindow.h kernel/qplatformwindow_p.h
        kernel/qrasterwindow.cpp kernel/qrasterwindow.h
        kernel/qscreen.cpp kernel/qscreen.h kernel/qscreen_p.h
        kernel/qsessionmanager.cpp kernel/qsessionmanager.h kernel/qsessionmanager_p.h
        kernel/qshortcutmap.cpp kernel/qshortcutmap_p.h
        kernel/qstylehints.cpp kernel/qstylehints.h
        kernel/qsurface.cpp kernel/qsurface.h
        kernel/qsurfaceformat.cpp kernel/qsurfaceformat.h
        kernel/qtestsupport_gui.cpp kernel/qtestsupport_gui.h
        kernel/qtguiglobal.h kernel/qtguiglobal_p.h
        kernel/qtouchdevice.cpp kernel/qtouchdevice.h kernel/qtouchdevice_p.h
        kernel/qwindow.cpp kernel/qwindow.h kernel/qwindow_p.h
        kernel/qwindowdefs.h
        kernel/qwindowsysteminterface.cpp kernel/qwindowsysteminterface.h kernel/qwindowsysteminterface_p.h
        math3d/qgenericmatrix.cpp math3d/qgenericmatrix.h
        math3d/qmatrix4x4.cpp math3d/qmatrix4x4.h
        math3d/qquaternion.cpp math3d/qquaternion.h
        math3d/qvector2d.cpp math3d/qvector2d.h
        math3d/qvector3d.cpp math3d/qvector3d.h
        math3d/qvector4d.cpp math3d/qvector4d.h
        painting/qbackingstore.cpp painting/qbackingstore.h
        painting/qbezier.cpp painting/qbezier_p.h
        painting/qblendfunctions.cpp painting/qblendfunctions_p.h
        painting/qblittable.cpp painting/qblittable_p.h
        painting/qbrush.cpp painting/qbrush.h
        painting/qcolor.cpp painting/qcolor.h painting/qcolor_p.h
        painting/qcolormatrix_p.h
        painting/qcolorspace.cpp painting/qcolorspace.h painting/qcolorspace_p.h
        painting/qcolortransferfunction_p.h
        painting/qcolortransfertable_p.h
        painting/qcolortransform.cpp painting/qcolortransform.h painting/qcolortransform_p.h
        painting/qcolortrc_p.h
        painting/qcolortrclut.cpp painting/qcolortrclut_p.h
        painting/qcompositionfunctions.cpp
        painting/qcosmeticstroker.cpp painting/qcosmeticstroker_p.h
        painting/qdatabuffer_p.h
        painting/qdrawhelper.cpp painting/qdrawhelper_p.h
        painting/qdrawhelper_x86_p.h
        painting/qdrawingprimitive_sse2_p.h
        painting/qemulationpaintengine.cpp painting/qemulationpaintengine_p.h
        painting/qfixed_p.h
        painting/qgrayraster.c painting/qgrayraster_p.h
        painting/qicc.cpp painting/qicc_p.h
        painting/qimagescale.cpp
        painting/qmatrix.cpp painting/qmatrix.h
        painting/qmemrotate.cpp painting/qmemrotate_p.h
        painting/qoutlinemapper.cpp painting/qoutlinemapper_p.h
        painting/qpagedpaintdevice.cpp painting/qpagedpaintdevice.h painting/qpagedpaintdevice_p.h
        painting/qpagelayout.cpp painting/qpagelayout.h
        painting/qpagesize.cpp painting/qpagesize.h
        painting/qpaintdevice.cpp painting/qpaintdevice.h
        painting/qpaintengine.cpp painting/qpaintengine.h painting/qpaintengine_p.h
        painting/qpaintengine_blitter.cpp painting/qpaintengine_blitter_p.h
        painting/qpaintengine_raster.cpp painting/qpaintengine_raster_p.h
        painting/qpaintengineex.cpp painting/qpaintengineex_p.h
        painting/qpainter.cpp painting/qpainter.h painting/qpainter_p.h
        painting/qpainterpath.cpp painting/qpainterpath.h painting/qpainterpath_p.h
        painting/qpathclipper.cpp painting/qpathclipper_p.h
        painting/qpathsimplifier.cpp painting/qpathsimplifier_p.h
        painting/qpdf.cpp painting/qpdf_p.h
        painting/qpdfwriter.cpp painting/qpdfwriter.h
        painting/qpen.cpp painting/qpen.h
        painting/qplatformbackingstore.cpp painting/qplatformbackingstore.h
        painting/qpolygon.cpp painting/qpolygon.h
        painting/qpolygonclipper_p.h
        painting/qrasterdefs_p.h
        painting/qrasterizer.cpp painting/qrasterizer_p.h
        painting/qrbtree_p.h
        painting/qregion.cpp painting/qregion.h
        painting/qrgb.h
        painting/qrgba64.h painting/qrgba64_p.h
        painting/qstroker.cpp painting/qstroker_p.h
        painting/qtextureglyphcache.cpp painting/qtextureglyphcache_p.h
        painting/qtransform.cpp painting/qtransform.h
        painting/qtriangulatingstroker.cpp painting/qtriangulatingstroker_p.h
        painting/qtriangulator.cpp painting/qtriangulator_p.h
        painting/qvectorpath_p.h
        text/qabstracttextdocumentlayout.cpp text/qabstracttextdocumentlayout.h text/qabstracttextdocumentlayout_p.h
        text/qdistancefield.cpp text/qdistancefield_p.h
        text/qfont.cpp text/qfont.h text/qfont_p.h
        text/qfontdatabase.cpp text/qfontdatabase.h
        text/qfontengine.cpp text/qfontengine_p.h
        text/qfontengine_qpf2.cpp
        text/qfontengineglyphcache.cpp text/qfontengineglyphcache_p.h
        text/qfontinfo.h
        text/qfontmetrics.cpp text/qfontmetrics.h
        text/qfontsubset.cpp text/qfontsubset_p.h
        text/qfragmentmap.cpp text/qfragmentmap_p.h
        text/qglyphrun.cpp text/qglyphrun.h text/qglyphrun_p.h
        text/qinputcontrol.cpp text/qinputcontrol_p.h
        text/qplatformfontdatabase.cpp text/qplatformfontdatabase.h
        text/qrawfont.cpp text/qrawfont.h text/qrawfont_p.h
        text/qstatictext.cpp text/qstatictext.h text/qstatictext_p.h
        text/qsyntaxhighlighter.cpp text/qsyntaxhighlighter.h
        text/qtextcursor.cpp text/qtextcursor.h text/qtextcursor_p.h
        text/qtextdocument.cpp text/qtextdocument.h text/qtextdocument_p.cpp text/qtextdocument_p.h
        text/qtextdocumentfragment.cpp text/qtextdocumentfragment.h text/qtextdocumentfragment_p.h
        text/qtextdocumentlayout.cpp text/qtextdocumentlayout_p.h
        text/qtextdocumentwriter.cpp text/qtextdocumentwriter.h
        text/qtextengine.cpp text/qtextengine_p.h
        text/qtextformat.cpp text/qtextformat.h text/qtextformat_p.h
        text/qtexthtmlparser.cpp text/qtexthtmlparser_p.h
        text/qtextimagehandler.cpp text/qtextimagehandler_p.h
        text/qtextlayout.cpp text/qtextlayout.h
        text/qtextlist.cpp text/qtextlist.h
        text/qtextobject.cpp text/qtextobject.h text/qtextobject_p.h
        text/qtextoption.cpp text/qtextoption.h
        text/qtexttable.cpp text/qtexttable.h text/qtexttable_p.h
        util/qabstractlayoutstyleinfo.cpp util/qabstractlayoutstyleinfo_p.h
        util/qastchandler.cpp util/qastchandler_p.h
        util/qdesktopservices.cpp util/qdesktopservices.h
        util/qgridlayoutengine.cpp util/qgridlayoutengine_p.h
        util/qhexstring_p.h
        util/qktxhandler.cpp util/qktxhandler_p.h
        util/qlayoutpolicy.cpp util/qlayoutpolicy_p.h
        util/qpkmhandler.cpp util/qpkmhandler_p.h
        util/qshaderformat.cpp util/qshaderformat_p.h
        util/qshadergraph.cpp util/qshadergraph_p.h
        util/qshadergraphloader.cpp util/qshadergraphloader_p.h
        util/qshaderlanguage.cpp util/qshaderlanguage_p.h
        util/qshadernode.cpp util/qshadernode_p.h
        util/qshadernodeport.cpp util/qshadernodeport_p.h
        util/qshadernodesloader.cpp util/qshadernodesloader_p.h
        util/qtexturefiledata.cpp util/qtexturefiledata_p.h
        util/qtexturefilehandler_p.h
        util/qtexturefilereader.cpp util/qtexturefilereader_p.h
        util/qvalidator.cpp util/qvalidator.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
        QT_QPA_DEFAULT_PLATFORM_NAME="${QT_QPA_DEFAULT_PLATFORM}" # special case
    LIBRARIES
        Qt::CorePrivate
        ZLIB::ZLIB # special case
    PUBLIC_LIBRARIES
        Qt::Core
)

# Resources:

# special case begin
# This name is fixed in the code:-/
add_qt_resource(Gui qmake_webgradients PREFIX "/qgradient" BASE "painting" FILES
    webgradients.binaryjson
)
# special case end

set_source_files_properties("painting/../../3rdparty/icc/sRGB2014.icc"
    PROPERTIES alias "sRGB2014.icc")
add_qt_resource(Gui "qpdf" PREFIX "qpdf/" BASE "painting" FILES
    ../../3rdparty/icc/sRGB2014.icc
    qpdfa_metadata.xml)

add_qt_simd_part(Gui SIMD sse2
    SOURCES
        painting/qdrawhelper_sse2.cpp
)

add_qt_simd_part(Gui SIMD ssse3
    SOURCES
        image/qimage_ssse3.cpp
        painting/qdrawhelper_ssse3.cpp
)

add_qt_simd_part(Gui SIMD sse4_1
    SOURCES
        painting/qdrawhelper_sse4.cpp
        painting/qimagescale_sse4.cpp
)

add_qt_simd_part(Gui SIMD neon
    SOURCES
        image/qimage_neon.cpp
        painting/qdrawhelper_neon.cpp painting/qdrawhelper_neon_p.h
        painting/qimagescale_neon.cpp
)

add_qt_simd_part(Gui SIMD mips_dsp
    SOURCES
        painting/qdrawhelper_mips_dsp.cpp painting/qdrawhelper_mips_dsp_p.h
        painting/qdrawhelper_mips_dsp_asm.S
        painting/qt_mips_asm_dsp_p.h
)

add_qt_simd_part(Gui SIMD mips_dspr2
    SOURCES
        image/qimage_mips_dspr2.cpp
        image/qimage_mips_dspr2_asm.S
        painting/qdrawhelper_mips_dspr2_asm.S
)

# special case begin
# Replace arch_haswell with avx2 feature. That is what is used in the code requireing
# this file, too!
add_qt_simd_part(Gui SIMD avx2
    SOURCES
        painting/qdrawhelper_avx2.cpp
)
# special case end

# special case begin
extend_target(Gui CONDITION QT_FEATURE_standarditemmodel
    SOURCES
        itemmodels/qstandarditemmodel.cpp itemmodels/qstandarditemmodel.h itemmodels/qstandarditemmodel_p.h
)
# special case end


#### Keys ignored in scope 1:.:.:gui.pro:<TRUE>:
# CONFIG = "simd" "optimize_full" "qt_tracepoints"
# MODULE_PLUGIN_TYPES = "platforms" "platforms/darwin" "xcbglintegrations" "platformthemes" "platforminputcontexts" "generic" "iconengines" "imageformats" "egldeviceintegrations"
# QMAKE_DYNAMIC_LIST_FILE = "$$PWD/QtGui.dynlist"
# QMAKE_LIBS = "$$QMAKE_LIBS_GUI"
# _LOADED = "qt_module" "cmake_functions"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:gui.pro:qtConfig(opengl._x_):
# MODULE_CONFIG = "opengl"

#### Keys ignored in scope 3:.:.:gui.pro:QT_FEATURE_angle:
# MODULE_AUX_INCLUDES = "\$\$QT_MODULE_INCLUDE_BASE/QtANGLE"

#### Keys ignored in scope 4:.:.:gui.pro:testcocoon:
# _LOADED = "testcocoon"

extend_target(Gui CONDITION APPLE_OSX
    LIBRARIES
        ${FWAppKit}
)

extend_target(Gui CONDITION APPLE
    SOURCES
        image/qimage_darwin.mm
        painting/qcoregraphics.mm painting/qcoregraphics_p.h
    LIBRARIES
        ${FWCoreGraphics}
)

extend_target(Gui CONDITION QT_FEATURE_animation
    SOURCES
        animation/qguivariantanimation.cpp
)

extend_target(Gui CONDITION WIN32
    SOURCES
        kernel/qwindowdefs_win.h
)

#### Keys ignored in scope 8:.:.:gui.pro:WIN32:
# CMAKE_WINDOWS_BUILD = "True"

#### Keys ignored in scope 11:.:.:gui.pro:QT_FEATURE_egl:
# CMAKE_EGL_LIBS = "$$cmakeProcessLibs$$QMAKE_LIBS_EGL"

#### Keys ignored in scope 12:.:.:gui.pro:NOT QMAKE_LIBDIR_EGL_ISEMPTY:
# CMAKE_EGL_LIBDIR = "$$cmakeTargetPath$$QMAKE_LIBDIR_EGL"

#### Keys ignored in scope 13:.:.:gui.pro:QT_FEATURE_opengles2:
# CMAKE_GL_HEADER_NAME = "GLES2/gl2.h"
# CMAKE_OPENGL_INCDIRS = "$$cmakePortablePaths$$QMAKE_INCDIR_OPENGL_ES2"
# CMAKE_OPENGL_LIBS = "$$cmakeProcessLibs$$QMAKE_LIBS_OPENGL_ES2"
# CMAKE_QT_OPENGL_IMPLEMENTATION = "GLESv2"

#### Keys ignored in scope 14:.:.:gui.pro:NOT QMAKE_INCDIR_OPENGL_ES2_ISEMPTY:
# CMAKE_GL_INCDIRS = "$$cmakeTargetPaths$$QMAKE_INCDIR_OPENGL_ES2"

#### Keys ignored in scope 15:.:.:gui.pro:NOT QMAKE_LIBDIR_OPENGL_ES2_ISEMPTY:
# CMAKE_OPENGL_LIBDIR = "$$cmakePortablePaths$$QMAKE_LIBDIR_OPENGL_ES2"

#### Keys ignored in scope 17:.:.:gui.pro:QT_FEATURE_opengl:
# CMAKE_GL_HEADER_NAME = "GL/gl.h"
# CMAKE_OPENGL_INCDIRS = "$$cmakePortablePaths$$QMAKE_INCDIR_OPENGL"
# CMAKE_QT_OPENGL_IMPLEMENTATION = "GL"

#### Keys ignored in scope 18:.:.:gui.pro:NOT QMAKE_INCDIR_OPENGL_ISEMPTY:
# CMAKE_GL_INCDIRS = "$$cmakeTargetPaths$$QMAKE_INCDIR_OPENGL"

#### Keys ignored in scope 19:.:.:gui.pro:NOT QT_FEATURE_dynamicgl:
# CMAKE_OPENGL_LIBS = "$$cmakeProcessLibs$$QMAKE_LIBS_OPENGL"

#### Keys ignored in scope 20:.:.:gui.pro:NOT QMAKE_LIBDIR_OPENGL_ISEMPTY:
# CMAKE_OPENGL_LIBDIR = "$$cmakePortablePaths$$QMAKE_LIBDIR_OPENGL"

#### Keys ignored in scope 21:.:.:gui.pro:APPLE_OSX:
# CMAKE_GL_HEADER_NAME = "gl.h"

#### Keys ignored in scope 22:.:.:gui.pro:QT_FEATURE_egl:
# CMAKE_EGL_INCDIRS = "$$cmakePortablePaths$$QMAKE_INCDIR_EGL"

extend_target(Gui CONDITION QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessible.cpp accessible/qaccessible.h
        accessible/qaccessiblebridge.cpp accessible/qaccessiblebridge.h
        accessible/qaccessiblecache.cpp accessible/qaccessiblecache_p.h
        accessible/qaccessibleobject.cpp accessible/qaccessibleobject.h
        accessible/qaccessibleplugin.cpp accessible/qaccessibleplugin.h
        accessible/qplatformaccessibility.cpp accessible/qplatformaccessibility.h
)

extend_target(Gui CONDITION APPLE_OSX AND QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessiblecache_mac.mm
    LIBRARIES
        ${FWFoundation}
)

extend_target(Gui CONDITION QT_FEATURE_draganddrop
    SOURCES
        kernel/qdnd.cpp kernel/qdnd_p.h
        kernel/qdrag.cpp kernel/qdrag.h
        kernel/qplatformdrag.cpp kernel/qplatformdrag.h
        kernel/qshapedpixmapdndwindow.cpp kernel/qshapedpixmapdndwindow_p.h
        kernel/qsimpledrag.cpp kernel/qsimpledrag_p.h
)

# special case begin
# With qmake, gui's opengl.pri used CONFIG += opengl, where opengl.prf
# acted like WrapOpenGL: direct linkage against either libGLESv2 or
# libGL, depending on the opengl _feature_. This is done by hand now
# here (where the feature is available).
# DO NOT MOVE THIS TO THE BEGINNING OF THE FILE, the feature variables are not
# available until the add_qt_module call.
qt_find_package(WrapOpenGL PROVIDED_TARGETS WrapOpenGL)
# special case end

extend_target(Gui CONDITION QT_FEATURE_opengl
    SOURCES
        kernel/qopenglcontext.cpp kernel/qopenglcontext.h kernel/qopenglcontext_p.h
        kernel/qopenglwindow.cpp kernel/qopenglwindow.h
        kernel/qplatformopenglcontext.cpp kernel/qplatformopenglcontext.h
        opengl/qopengl.cpp opengl/qopengl.h opengl/qopengl_p.h
        opengl/qopengl2pexvertexarray.cpp opengl/qopengl2pexvertexarray_p.h
        opengl/qopenglbuffer.cpp opengl/qopenglbuffer.h
        opengl/qopenglcustomshaderstage.cpp opengl/qopenglcustomshaderstage_p.h
        opengl/qopengldebug.cpp opengl/qopengldebug.h
        opengl/qopenglengineshadermanager.cpp opengl/qopenglengineshadermanager_p.h
        opengl/qopenglengineshadersource_p.h
        opengl/qopenglextensions_p.h
        opengl/qopenglextrafunctions.h
        opengl/qopenglframebufferobject.cpp opengl/qopenglframebufferobject.h opengl/qopenglframebufferobject_p.h
        opengl/qopenglfunctions.cpp opengl/qopenglfunctions.h
        opengl/qopenglgradientcache.cpp opengl/qopenglgradientcache_p.h
        opengl/qopenglpaintdevice.cpp opengl/qopenglpaintdevice.h opengl/qopenglpaintdevice_p.h
        opengl/qopenglpaintengine.cpp opengl/qopenglpaintengine_p.h
        opengl/qopenglpixeltransferoptions.cpp opengl/qopenglpixeltransferoptions.h
        opengl/qopenglprogrambinarycache.cpp opengl/qopenglprogrambinarycache_p.h
        opengl/qopenglshadercache_p.h
        opengl/qopenglshaderprogram.cpp opengl/qopenglshaderprogram.h
        opengl/qopengltexture.cpp opengl/qopengltexture.h opengl/qopengltexture_p.h
        opengl/qopengltextureblitter.cpp opengl/qopengltextureblitter.h
        opengl/qopengltexturecache.cpp opengl/qopengltexturecache_p.h
        opengl/qopengltextureglyphcache.cpp opengl/qopengltextureglyphcache_p.h
        opengl/qopengltexturehelper.cpp opengl/qopengltexturehelper_p.h
        opengl/qopengltextureuploader.cpp opengl/qopengltextureuploader_p.h
        opengl/qopenglversionfunctions.cpp opengl/qopenglversionfunctions.h
        opengl/qopenglversionfunctionsfactory.cpp opengl/qopenglversionfunctionsfactory_p.h
        opengl/qopenglvertexarrayobject.cpp opengl/qopenglvertexarrayobject.h
    LIBRARIES # special case
        WrapOpenGL # special case
)

extend_target(Gui CONDITION QT_FEATURE_movie
    SOURCES
        image/qmovie.cpp image/qmovie.h
)

extend_target(Gui CONDITION WIN32 AND NOT WINRT
    SOURCES
        image/qpixmap_win.cpp
)

extend_target(Gui CONDITION QT_FEATURE_png
    SOURCES
        image/qpnghandler.cpp image/qpnghandler_p.h
    LIBRARIES
        PNG::PNG
)

extend_target(Gui CONDITION QT_FEATURE_harfbuzz
    SOURCES
        text/qharfbuzzng.cpp text/qharfbuzzng_p.h
    LIBRARIES
        harfbuzz::harfbuzz
)

extend_target(Gui CONDITION QT_FEATURE_textodfwriter
    SOURCES
        text/qtextodfwriter.cpp text/qtextodfwriter_p.h
        text/qzip.cpp
        text/qzipreader_p.h
        text/qzipwriter_p.h
)

extend_target(Gui CONDITION QT_FEATURE_textmarkdownreader
    SOURCES
        text/qtextmarkdownimporter.cpp text/qtextmarkdownimporter_p.h
)

extend_target(Gui CONDITION QT_FEATURE_system_textmarkdownreader AND QT_FEATURE_textmarkdownreader
    PUBLIC_LIBRARIES
        libmd4c
)

extend_target(Gui CONDITION QT_FEATURE_textmarkdownreader AND NOT QT_FEATURE_system_textmarkdownreader
    SOURCES
        ../3rdparty/md4c/md4c.c ../3rdparty/md4c/md4c.h
    INCLUDE_DIRECTORIES
        ../3rdparty/md4c
)

extend_target(Gui CONDITION QT_FEATURE_textmarkdownwriter
    SOURCES
        text/qtextmarkdownwriter.cpp text/qtextmarkdownwriter_p.h
)

extend_target(Gui CONDITION QT_FEATURE_cssparser
    SOURCES
        painting/qcssutil.cpp
        text/qcssparser.cpp text/qcssparser_p.h
)

# extend_target(Gui CONDITION GCC AND QT_GCC_MAJOR_VERSION___equals___5 ... # special case:

#### Keys ignored in scope 50:.:painting:painting/painting.pri:NOT APPLE_UIKIT AND NOT WIN32 AND QT_ARCH___contains___arm:
# CONFIG = "no_clang_integrated_as"

extend_target(Gui CONDITION UNIX AND NOT APPLE_UIKIT AND NOT INTEGRITY AND NOT (TEST_architecture_arch STREQUAL "arm64")
    DEFINES
        ENABLE_PIXMAN_DRAWHELPERS
)

# special case begin
extend_target(Gui CONDITION UNIX AND NOT APPLE_UIKIT AND NOT INTEGRITY AND NOT (TEST_architecture_arch STREQUAL "arm64" AND QT_FEATURE_neon)
    SOURCES
        ../3rdparty/pixman/pixman-arm-neon-asm.S
        painting/qdrawhelper_neon_asm.S
)
# special case end

#### Keys ignored in scope 51:.:painting:painting/painting.pri:NOT APPLE_UIKIT AND NOT WIN32 AND NOT INTEGRITY AND NOT QT_ARCH___contains___arm64:
# NEON_ASM = "../3rdparty/pixman/pixman-arm-neon-asm.S" "painting/qdrawhelper_neon_asm.S"

extend_target(Gui CONDITION QT_FEATURE_regularexpression
    SOURCES
        util/qshadergenerator.cpp util/qshadergenerator_p.h
)

#### Keys ignored in scope 61:.:opengl:opengl/opengl.pri:QT_FEATURE_opengles2:
# CONFIG = "opengles2"

extend_target(Gui CONDITION QT_FEATURE_opengl AND NOT QT_FEATURE_opengles2
    SOURCES
        opengl/qopenglfunctions_1_0.cpp opengl/qopenglfunctions_1_0.h
        opengl/qopenglfunctions_1_1.cpp opengl/qopenglfunctions_1_1.h
        opengl/qopenglfunctions_1_2.cpp opengl/qopenglfunctions_1_2.h
        opengl/qopenglfunctions_1_3.cpp opengl/qopenglfunctions_1_3.h
        opengl/qopenglfunctions_1_4.cpp opengl/qopenglfunctions_1_4.h
        opengl/qopenglfunctions_1_5.cpp opengl/qopenglfunctions_1_5.h
        opengl/qopenglfunctions_2_0.cpp opengl/qopenglfunctions_2_0.h
        opengl/qopenglfunctions_2_1.cpp opengl/qopenglfunctions_2_1.h
        opengl/qopenglfunctions_3_0.cpp opengl/qopenglfunctions_3_0.h
        opengl/qopenglfunctions_3_1.cpp opengl/qopenglfunctions_3_1.h
        opengl/qopenglfunctions_3_2_compatibility.cpp opengl/qopenglfunctions_3_2_compatibility.h
        opengl/qopenglfunctions_3_2_core.cpp opengl/qopenglfunctions_3_2_core.h
        opengl/qopenglfunctions_3_3_compatibility.cpp opengl/qopenglfunctions_3_3_compatibility.h
        opengl/qopenglfunctions_3_3_core.cpp opengl/qopenglfunctions_3_3_core.h
        opengl/qopenglfunctions_4_0_compatibility.cpp opengl/qopenglfunctions_4_0_compatibility.h
        opengl/qopenglfunctions_4_0_core.cpp opengl/qopenglfunctions_4_0_core.h
        opengl/qopenglfunctions_4_1_compatibility.cpp opengl/qopenglfunctions_4_1_compatibility.h
        opengl/qopenglfunctions_4_1_core.cpp opengl/qopenglfunctions_4_1_core.h
        opengl/qopenglfunctions_4_2_compatibility.cpp opengl/qopenglfunctions_4_2_compatibility.h
        opengl/qopenglfunctions_4_2_core.cpp opengl/qopenglfunctions_4_2_core.h
        opengl/qopenglfunctions_4_3_compatibility.cpp opengl/qopenglfunctions_4_3_compatibility.h
        opengl/qopenglfunctions_4_3_core.cpp opengl/qopenglfunctions_4_3_core.h
        opengl/qopenglfunctions_4_4_compatibility.cpp opengl/qopenglfunctions_4_4_compatibility.h
        opengl/qopenglfunctions_4_4_core.cpp opengl/qopenglfunctions_4_4_core.h
        opengl/qopenglfunctions_4_5_compatibility.cpp opengl/qopenglfunctions_4_5_compatibility.h
        opengl/qopenglfunctions_4_5_core.cpp opengl/qopenglfunctions_4_5_core.h
        opengl/qopenglqueryhelper_p.h
        opengl/qopengltimerquery.cpp opengl/qopengltimerquery.h
)

extend_target(Gui CONDITION QT_FEATURE_opengl AND QT_FEATURE_opengles2
    SOURCES
        opengl/qopenglfunctions_es2.cpp opengl/qopenglfunctions_es2.h
)

# special case begin
# Make Vulkan a global imported target, so that generator expressions involving Vulkan
# can be correctly evaluated in any sub project directory.
if(TARGET Vulkan::Vulkan)
    set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_GLOBAL TRUE)
endif()
# special case end

extend_target(Gui CONDITION QT_FEATURE_vulkan
    SOURCES
        vulkan/qplatformvulkaninstance.cpp vulkan/qplatformvulkaninstance.h
        vulkan/qvulkanfunctions.cpp
        vulkan/qvulkaninstance.cpp vulkan/qvulkaninstance.h
        vulkan/qvulkanwindow.cpp vulkan/qvulkanwindow.h vulkan/qvulkanwindow_p.h
    PUBLIC_LIBRARIES
        Vulkan::Vulkan_nolink
)

# special case begin
if (QT_FEATURE_vulkan)
    set(qvkgen_command "${HOST_QVKGEN}")
    if (TARGET qvkgen)
        set(qvkgen_command qvkgen)
    endif()

    set(vulkan_fun "qvulkanfunctions.h")
    set(vulkan_fun_p "qvulkanfunctions_p.h")
    set(vulkan_fun_p_cpp "qvulkanfunctions_p.cpp")

    add_custom_command(
        OUTPUT "vulkan/${vulkan_fun}" "vulkan/${vulkan_fun_p}" "vulkan/${vulkan_fun_p_cpp}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/vulkan/"
        COMMAND "${qvkgen_command}"
            "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk.xml"
            "${PROJECT_SOURCE_DIR}/header.LGPL"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/qvulkanfunctions"
        DEPENDS vulkan/vk.xml
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating vulkan data"
    )

    extend_target(Gui CONDITION QT_FEATURE_vulkan
        SOURCES
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${vulkan_fun}"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${vulkan_fun_p}"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${vulkan_fun_p_cpp}"
    )
endif()
# special case end

#### Keys ignored in scope 68:.:vulkan:vulkan/vulkan.pri:QT_FEATURE_vulkan:
# CONFIG = "generated_privates"

#### Keys ignored in scope 70:.:vulkan:vulkan/vulkan.pri:else:
# qvkgen_h.CONFIG = "target_predeps" "no_link"

extend_target(Gui CONDITION WASM
    SOURCES
        platform/wasm/qwasmlocalfileaccess.cpp platform/wasm/qwasmlocalfileaccess_p.h
)

# special case begin
# qpa headers are expected to be located right next to QtGui's private
# headers. So a private QtGui header is #include <private/qfoo_p.h> and
# a qpa header is #include <qpa/qplatformfoo.h>, both of them implying
# linkage against Qt::GuiPrivate.
qt_read_headers_pri("Gui" "module_headers")

# No need to copy these in a non-prefix build, syncqt.pl takes care of generating the files
# in the build dir.
qt_install(FILES ${module_headers_qpa}
    DESTINATION ${INSTALL_INCLUDEDIR}/QtGui/${PROJECT_VERSION}/QtGui/qpa)
# special case end

qt_create_tracepoints(Gui qtgui.tracepoints)
add_qt_docs(
    doc/qtgui.qdocconf
)
