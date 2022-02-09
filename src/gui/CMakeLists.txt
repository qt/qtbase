# Generated from gui.pro.

#####################################################################
## Gui Module:
#####################################################################

# special case begin
qt_find_package(X11_XCB)
qt_find_package(WrapHarfbuzz PROVIDED_TARGETS WrapHarfbuzz::WrapHarfbuzz)
qt_find_package(WrapPNG PROVIDED_TARGETS WrapPNG::WrapPNG)
qt_find_package(WrapFreetype PROVIDED_TARGETS WrapFreetype::WrapFreetype)

if (QT_FEATURE_gui)
    if(WIN32)
        set(_default_platform "windows")
    elseif(ANDROID)
        set(_default_platform "android")
    elseif(MACOS)
        set(_default_platform "cocoa")
    elseif(TVOS OR IOS)
        set(_default_platform "ios")
    elseif(WATCHOS)
        set(_default_platform "minimal")
    elseif(QNX)
        set(_default_platform "qnx")
    elseif(INTEGRITY)
        set(_default_platform "eglfs")
    elseif(HAIKU)
        set(_default_platform "haiku")
    elseif(WASM)
        set(_default_platform "wasm")
    else()
        set(_default_platform "xcb")
    endif()

    set(QT_QPA_DEFAULT_PLATFORM "${_default_platform}" CACHE STRING "QPA default platform")
endif()

# Silence warnings in 3rdparty code
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties(../3rdparty/md4c/md4c.c PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set_source_files_properties(../3rdparty/md4c/md4c.c PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers -Wno-missing-braces")
endif()

unset(qmake_module_config)
if(QT_FEATURE_opengl)
    list(APPEND qmake_module_config opengl)
endif()

# special case end

qt_internal_add_module(Gui
    PLUGIN_TYPES accessiblebridge platforms platforms/darwin xcbglintegrations platformthemes platforminputcontexts generic iconengines imageformats egldeviceintegrations
    FEATURE_DEPENDENCIES  # special case:
        Qt::Network # special case:
    QMAKE_MODULE_CONFIG "${qmake_module_config}" # special case
    SOURCES
        image/qabstractfileiconengine.cpp image/qabstractfileiconengine_p.h
        image/qabstractfileiconprovider.cpp image/qabstractfileiconprovider.h image/qabstractfileiconprovider_p.h
        image/qbitmap.cpp image/qbitmap.h
        image/qbmphandler.cpp image/qbmphandler_p.h
        image/qicon.cpp image/qicon.h image/qicon_p.h
        image/qiconengine.cpp image/qiconengine.h
        image/qiconengineplugin.cpp image/qiconengineplugin.h
        image/qiconloader.cpp image/qiconloader_p.h
        image/qimage.cpp image/qimage.h image/qimage_p.h
        image/qimage_conversions.cpp
        image/qimageiohandler.cpp image/qimageiohandler.h
        image/qimagepixmapcleanuphooks.cpp image/qimagepixmapcleanuphooks_p.h
        image/qimagereader.cpp image/qimagereader.h
        image/qimagereaderwriterhelpers.cpp image/qimagereaderwriterhelpers_p.h
        image/qimagewriter.cpp image/qimagewriter.h
        image/qpaintengine_pic.cpp image/qpaintengine_pic_p.h
        image/qpicture.cpp image/qpicture.h image/qpicture_p.h
        image/qpixmap.cpp image/qpixmap.h
        image/qpixmap_blitter.cpp image/qpixmap_blitter_p.h
        image/qpixmap_raster.cpp image/qpixmap_raster_p.h
        image/qpixmapcache.cpp image/qpixmapcache.h image/qpixmapcache_p.h
        image/qplatformpixmap.cpp image/qplatformpixmap.h
        image/qppmhandler.cpp image/qppmhandler_p.h
        image/qxbmhandler.cpp image/qxbmhandler_p.h
        image/qxpmhandler.cpp image/qxpmhandler_p.h
        kernel/qclipboard.cpp kernel/qclipboard.h
        kernel/qcursor.cpp kernel/qcursor.h kernel/qcursor_p.h
        kernel/qeventpoint.cpp kernel/qeventpoint.h kernel/qeventpoint_p.h
        kernel/qevent.cpp kernel/qevent.h kernel/qevent_p.h
        kernel/qgenericplugin.cpp kernel/qgenericplugin.h
        kernel/qgenericpluginfactory.cpp kernel/qgenericpluginfactory.h
        kernel/qguiapplication.cpp kernel/qguiapplication.h kernel/qguiapplication_p.h kernel/qguiapplication_platform.h
        kernel/qguivariant.cpp
        kernel/qhighdpiscaling.cpp kernel/qhighdpiscaling_p.h
        kernel/qinputdevice.cpp kernel/qinputdevice.h kernel/qinputdevice_p.h
        kernel/qinputdevicemanager.cpp kernel/qinputdevicemanager_p.h
        kernel/qinputdevicemanager_p_p.h
        kernel/qinputmethod.cpp kernel/qinputmethod.h kernel/qinputmethod_p.h
        kernel/qinternalmimedata.cpp kernel/qinternalmimedata_p.h
        kernel/qkeymapper.cpp kernel/qkeymapper_p.h
        kernel/qoffscreensurface.cpp kernel/qoffscreensurface.h kernel/qoffscreensurface_p.h
        kernel/qoffscreensurface_platform.h
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
        kernel/qplatformmenu.cpp kernel/qplatformmenu.h kernel/qplatformmenu_p.h
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
        kernel/qpointingdevice.cpp kernel/qpointingdevice.h kernel/qpointingdevice_p.h
        kernel/qrasterwindow.cpp kernel/qrasterwindow.h
        kernel/qscreen.cpp kernel/qscreen.h kernel/qscreen_p.h
        kernel/qsessionmanager.cpp kernel/qsessionmanager.h kernel/qsessionmanager_p.h
        kernel/qstylehints.cpp kernel/qstylehints.h
        kernel/qsurface.cpp kernel/qsurface.h
        kernel/qsurfaceformat.cpp kernel/qsurfaceformat.h
        kernel/qtestsupport_gui.cpp kernel/qtestsupport_gui.h
        kernel/qtguiglobal.h kernel/qtguiglobal_p.h
        kernel/qwindow.cpp kernel/qwindow.h kernel/qwindow_p.h
        kernel/qwindowdefs.h
        kernel/qwindowsysteminterface.cpp kernel/qwindowsysteminterface.h kernel/qwindowsysteminterface_p.h
        math3d/qgenericmatrix.cpp math3d/qgenericmatrix.h
        math3d/qmatrix4x4.cpp math3d/qmatrix4x4.h
        math3d/qquaternion.cpp math3d/qquaternion.h
        math3d/qvector2d.h
        math3d/qvector3d.h
        math3d/qvector4d.h
        math3d/qvectornd.cpp math3d/qvectornd.h
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
        painting/qdrawhelper_p.h
        painting/qdrawhelper_x86_p.h
        painting/qdrawingprimitive_sse2_p.h
        painting/qemulationpaintengine.cpp painting/qemulationpaintengine_p.h
        painting/qfixed_p.h
        painting/qgrayraster.c painting/qgrayraster_p.h
        painting/qicc.cpp painting/qicc_p.h
        painting/qimagescale.cpp
        painting/qmemrotate.cpp painting/qmemrotate_p.h
        painting/qoutlinemapper.cpp painting/qoutlinemapper_p.h
        painting/qpagedpaintdevice.cpp painting/qpagedpaintdevice.h painting/qpagedpaintdevice_p.h
        painting/qpagelayout.cpp painting/qpagelayout.h
        painting/qpageranges.cpp painting/qpageranges.h painting/qpageranges_p.h
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
        painting/qpixellayout.cpp painting/qpixellayout_p.h
        painting/qplatformbackingstore.cpp painting/qplatformbackingstore.h
        painting/qpolygon.cpp painting/qpolygon.h
        painting/qrasterdefs_p.h
        painting/qrasterizer.cpp painting/qrasterizer_p.h
        painting/qrbtree_p.h
        painting/qregion.cpp painting/qregion.h
        painting/qrgb.h
        painting/qrgba64.h painting/qrgba64_p.h
        painting/qrgbafloat.h
        painting/qstroker.cpp painting/qstroker_p.h
        painting/qtextureglyphcache.cpp painting/qtextureglyphcache_p.h
        painting/qtransform.cpp painting/qtransform.h
        painting/qtriangulatingstroker.cpp painting/qtriangulatingstroker_p.h
        painting/qtriangulator.cpp painting/qtriangulator_p.h
        painting/qvectorpath_p.h
        rhi/qrhi.cpp rhi/qrhi_p.h
        rhi/qrhi_p_p.h
        rhi/qrhinull.cpp rhi/qrhinull_p.h
        rhi/qrhinull_p_p.h
        rhi/qrhiprofiler.cpp rhi/qrhiprofiler_p.h
        rhi/qrhiprofiler_p_p.h
        rhi/qshader.cpp rhi/qshader_p.h
        rhi/qshader_p_p.h
        rhi/qshaderdescription.cpp rhi/qshaderdescription_p.h
        rhi/qshaderdescription_p_p.h
        text/qabstracttextdocumentlayout.cpp text/qabstracttextdocumentlayout.h text/qabstracttextdocumentlayout_p.h
        text/qdistancefield.cpp text/qdistancefield_p.h
        text/qfont.cpp text/qfont.h text/qfont_p.h
        text/qfontdatabase.cpp text/qfontdatabase.h text/qfontdatabase_p.h
        text/qfontengine.cpp text/qfontengine_p.h
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
        util/qtexturefiledata.cpp util/qtexturefiledata_p.h
        util/qtexturefilehandler_p.h
        util/qtexturefilereader.cpp util/qtexturefilereader_p.h
        util/qvalidator.cpp util/qvalidator.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
        QT_QPA_DEFAULT_PLATFORM_NAME="${QT_QPA_DEFAULT_PLATFORM}" # special case
    INCLUDE_DIRECTORIES
        ../3rdparty/VulkanMemoryAllocator
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    NO_PCH_SOURCES # special case
        "painting/qdrawhelper.cpp" # special case
    PRECOMPILED_HEADER
        "kernel/qt_gui_pch.h"
)

# Resources:
set_source_files_properties("../3rdparty/icc/sRGB2014.icc"
    PROPERTIES QT_RESOURCE_ALIAS "sRGB2014.icc"
)
set(qpdf_resource_files
    "../3rdparty/icc/sRGB2014.icc"
    "painting/qpdfa_metadata.xml"
)

qt_internal_add_resource(Gui "qpdf"
    PREFIX
        "/qpdf/"
    BASE
        "painting"
    FILES
        ${qpdf_resource_files}
)

if(QT_FEATURE_reduce_relocations AND UNIX AND GCC)
    target_link_options(Gui PRIVATE
                        "LINKER:--dynamic-list=${CMAKE_CURRENT_LIST_DIR}/QtGui.dynlist")
endif()

# special case begin
qt_internal_extend_target(Gui CONDITION QT_FEATURE_standarditemmodel
    SOURCES
        itemmodels/qstandarditemmodel.cpp itemmodels/qstandarditemmodel.h itemmodels/qstandarditemmodel_p.h
)
# special case end


#### Keys ignored in scope 1:.:.:gui.pro:<TRUE>:
# QMAKE_LIBS = "$$QMAKE_LIBS_GUI"

## Scopes:
#####################################################################

# special case begin
# With qmake, gui's opengl.pri used CONFIG += opengl, where opengl.prf
# used direct public linkage against either libGLESv2 or libGL, depending
# on the opengl _feature_. This is done by hand now here (where the
# feature is available).
# On Windows with 'dynamic' OpenGL we must not link to opengl32.
# DO NOT MOVE THIS TO THE BEGINNING OF THE FILE, the feature variables are not
# available until the qt_internal_add_module call.
if(QT_FEATURE_opengl)
    if(QT_FEATURE_opengles2)
        find_package(GLESv2)
        target_link_libraries(Gui PUBLIC GLESv2::GLESv2)
    elseif(NOT QT_FEATURE_opengl_dynamic)
        target_link_libraries(Gui PUBLIC WrapOpenGL::WrapOpenGL)
    endif()
endif()
# special case end

qt_internal_extend_target(Gui CONDITION QT_FEATURE_opengl
    SOURCES
        kernel/qopenglcontext.cpp kernel/qopenglcontext.h kernel/qopenglcontext_p.h
        kernel/qopenglcontext_platform.h
        kernel/qplatformopenglcontext.cpp kernel/qplatformopenglcontext.h
        opengl/qopengl.cpp opengl/qopengl.h opengl/qopengl_p.h
        opengl/qopenglextensions_p.h
        opengl/qopenglextrafunctions.h
        opengl/qopenglfunctions.cpp opengl/qopenglfunctions.h
        opengl/qopenglprogrambinarycache.cpp opengl/qopenglprogrambinarycache_p.h
        rhi/qrhigles2.cpp rhi/qrhigles2_p.h
        rhi/qrhigles2_p_p.h
)

#### Keys ignored in scope 2:.:.:gui.pro:QT_FEATURE_opengl:
# MODULE_CONFIG = "opengl"

qt_internal_extend_target(Gui CONDITION MACOS
    SOURCES
        platform/macos/qcocoanativeinterface.mm
    LIBRARIES
        ${FWAppKit}
        ${FWCarbon}
    PUBLIC_LIBRARIES
        ${FWAppKit}
)

qt_internal_extend_target(Gui CONDITION APPLE
    SOURCES
        image/qimage_darwin.mm
        painting/qcoregraphics.mm painting/qcoregraphics_p.h
        painting/qrasterbackingstore.cpp painting/qrasterbackingstore_p.h
        platform/darwin/qmacmime.mm platform/darwin/qmacmime_p.h
        platform/darwin/qapplekeymapper.mm platform/darwin/qapplekeymapper_p.h
        text/coretext/qcoretextfontdatabase.mm text/coretext/qcoretextfontdatabase_p.h
        text/coretext/qfontengine_coretext.mm text/coretext/qfontengine_coretext_p.h
    LIBRARIES
        ${FWCoreFoundation}
        ${FWCoreGraphics}
        ${FWCoreText}
        ${FWFoundation}
    PUBLIC_LIBRARIES
        ${FWImageIO}
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_animation
    SOURCES
        animation/qguivariantanimation.cpp
)

qt_internal_extend_target(Gui CONDITION WIN32
    SOURCES
        image/qpixmap_win.cpp
        kernel/qwindowdefs_win.h
        platform/windows/qwindowsguieventdispatcher.cpp platform/windows/qwindowsguieventdispatcher_p.h
        platform/windows/qwindowsmime_p.h
        platform/windows/qwindowsnativeinterface.cpp
        rhi/qrhid3d11.cpp rhi/qrhid3d11_p.h
        rhi/qrhid3d11_p_p.h
        text/windows/qwindowsfontdatabase.cpp text/windows/qwindowsfontdatabase_p.h
        text/windows/qwindowsfontdatabasebase.cpp text/windows/qwindowsfontdatabasebase_p.h
        text/windows/qwindowsfontengine.cpp text/windows/qwindowsfontengine_p.h
        text/windows/qwindowsnativeimage.cpp text/windows/qwindowsnativeimage_p.h
    LIBRARIES
        advapi32
        gdi32
        ole32
        shell32
        user32
    PUBLIC_LIBRARIES
        d3d11
        dxgi
        dxguid
)

#### Keys ignored in scope 7:.:.:gui.pro:WIN32:
# CMAKE_WINDOWS_BUILD = "True"

# special case begin
if(QT_FEATURE_egl)
    qt_find_package(EGL)
endif()
# special case end

qt_internal_extend_target(Gui CONDITION QT_FEATURE_egl
    SOURCES
        opengl/platform/egl/qeglconvenience.cpp opengl/platform/egl/qeglconvenience_p.h
        opengl/platform/egl/qeglstreamconvenience.cpp opengl/platform/egl/qeglstreamconvenience_p.h
        opengl/platform/egl/qt_egl_p.h
# special case begin
    LIBRARIES
        EGL::EGL # special case
# special case end
)

#### Keys ignored in scope 8:.:.:gui.pro:QT_FEATURE_egl:
# CMAKE_EGL_LIBS = "$$cmakeProcessLibs($$QMAKE_LIBS_EGL)"

#### Keys ignored in scope 9:.:.:gui.pro:NOT QMAKE_LIBDIR_EGL_ISEMPTY:
# CMAKE_EGL_LIBDIR = "$$cmakeTargetPath($$QMAKE_LIBDIR_EGL)"

#### Keys ignored in scope 10:.:.:gui.pro:QT_FEATURE_opengles2:
# CMAKE_GL_HEADER_NAME = "GLES2/gl2.h"
# CMAKE_OPENGL_INCDIRS = "$$cmakePortablePaths($$QMAKE_INCDIR_OPENGL_ES2)"
# CMAKE_OPENGL_LIBS = "$$cmakeProcessLibs($$QMAKE_LIBS_OPENGL_ES2)"
# CMAKE_QT_OPENGL_IMPLEMENTATION = "GLESv2"

#### Keys ignored in scope 11:.:.:gui.pro:NOT QMAKE_INCDIR_OPENGL_ES2_ISEMPTY:
# CMAKE_GL_INCDIRS = "$$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL_ES2)"

#### Keys ignored in scope 12:.:.:gui.pro:NOT QMAKE_LIBDIR_OPENGL_ES2_ISEMPTY:
# CMAKE_OPENGL_LIBDIR = "$$cmakePortablePaths($$QMAKE_LIBDIR_OPENGL_ES2)"

#### Keys ignored in scope 14:.:.:gui.pro:QT_FEATURE_opengl:
# CMAKE_GL_HEADER_NAME = "GL/gl.h"
# CMAKE_OPENGL_INCDIRS = "$$cmakePortablePaths($$QMAKE_INCDIR_OPENGL)"
# CMAKE_QT_OPENGL_IMPLEMENTATION = "GL"

#### Keys ignored in scope 15:.:.:gui.pro:NOT QMAKE_INCDIR_OPENGL_ISEMPTY:
# CMAKE_GL_INCDIRS = "$$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL)"

#### Keys ignored in scope 16:.:.:gui.pro:NOT QT_FEATURE_dynamicgl:
# CMAKE_OPENGL_LIBS = "$$cmakeProcessLibs($$QMAKE_LIBS_OPENGL)"

#### Keys ignored in scope 17:.:.:gui.pro:NOT QMAKE_LIBDIR_OPENGL_ISEMPTY:
# CMAKE_OPENGL_LIBDIR = "$$cmakePortablePaths($$QMAKE_LIBDIR_OPENGL)"

#### Keys ignored in scope 18:.:.:gui.pro:APPLE:
# CMAKE_GL_HEADER_NAME = "gl.h"

qt_internal_extend_target(Gui CONDITION QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessible.cpp accessible/qaccessible.h
        accessible/qaccessiblebridge.cpp accessible/qaccessiblebridge.h
        accessible/qaccessiblebridgeutils.cpp accessible/qaccessiblebridgeutils_p.h
        accessible/qaccessiblecache.cpp accessible/qaccessiblecache_p.h
        accessible/qaccessibleobject.cpp accessible/qaccessibleobject.h
        accessible/qaccessibleplugin.cpp accessible/qaccessibleplugin.h
        accessible/qplatformaccessibility.cpp accessible/qplatformaccessibility.h
)

qt_internal_extend_target(Gui CONDITION APPLE AND QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessiblecache_mac.mm
    LIBRARIES
        ${FWFoundation}
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_accessibility AND WIN32
    SOURCES
        accessible/windows/apisupport/qwindowsuiawrapper.cpp accessible/windows/apisupport/qwindowsuiawrapper_p.h
        accessible/windows/apisupport/uiaattributeids_p.h
        accessible/windows/apisupport/uiaclientinterfaces_p.h
        accessible/windows/apisupport/uiacontroltypeids_p.h
        accessible/windows/apisupport/uiaerrorids_p.h
        accessible/windows/apisupport/uiaeventids_p.h
        accessible/windows/apisupport/uiageneralids_p.h
        accessible/windows/apisupport/uiapatternids_p.h
        accessible/windows/apisupport/uiapropertyids_p.h
        accessible/windows/apisupport/uiaserverinterfaces_p.h
        accessible/windows/apisupport/uiatypes_p.h
)

if(QT_FEATURE_accessibility AND QT_FEATURE_accessibility_atspi_bridge)
    set(atspi_accessibility ON)
else()
    set(atspi_accessibility OFF)
endif()

qt_internal_extend_target(Gui CONDITION atspi_accessibility
    SOURCES
        accessible/linux/atspiadaptor.cpp accessible/linux/atspiadaptor_p.h
        accessible/linux/dbusconnection.cpp accessible/linux/dbusconnection_p.h
        accessible/linux/qspi_constant_mappings.cpp accessible/linux/qspi_constant_mappings_p.h
        accessible/linux/qspi_struct_marshallers.cpp accessible/linux/qspi_struct_marshallers_p.h
        accessible/linux/qspiaccessiblebridge.cpp accessible/linux/qspiaccessiblebridge_p.h
        accessible/linux/qspiapplicationadaptor.cpp accessible/linux/qspiapplicationadaptor_p.h
        accessible/linux/qspidbuscache.cpp accessible/linux/qspidbuscache_p.h
# special case begin
    DBUS_ADAPTOR_SOURCES
        accessible/linux/dbusxml/Cache.xml
        accessible/linux/dbusxml/DeviceEventController.xml
    DBUS_ADAPTOR_FLAGS
        "-i" "QtGui/private/qspi_struct_marshallers_p.h"
    DBUS_INTERFACE_SOURCES
        accessible/linux/dbusxml/Bus.xml
        accessible/linux/dbusxml/Socket.xml
    DBUS_INTERFACE_FLAGS
        "-i" "QtGui/private/qspi_struct_marshallers_p.h"
# special case end
)

if(atspi_accessibility)
    qt_internal_add_target_include_dirs(Gui PkgConfig::ATSPI2)
endif()

qt_internal_extend_target(Gui CONDITION QT_FEATURE_action
    SOURCES
        kernel/qaction.cpp kernel/qaction.h kernel/qaction_p.h
        kernel/qactiongroup.cpp kernel/qactiongroup.h kernel/qactiongroup_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_draganddrop
    SOURCES
        kernel/qdnd.cpp kernel/qdnd_p.h
        kernel/qdrag.cpp kernel/qdrag.h
        kernel/qplatformdrag.cpp kernel/qplatformdrag.h
        kernel/qshapedpixmapdndwindow.cpp kernel/qshapedpixmapdndwindow_p.h
        kernel/qsimpledrag.cpp kernel/qsimpledrag_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_shortcut
    SOURCES
        kernel/qkeysequence.cpp kernel/qkeysequence.h kernel/qkeysequence_p.h
        kernel/qshortcut.cpp kernel/qshortcut.h kernel/qshortcut_p.h
        kernel/qshortcutmap.cpp kernel/qshortcutmap_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_movie
    SOURCES
        image/qmovie.cpp image/qmovie.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_png
    SOURCES
        image/qpnghandler.cpp image/qpnghandler_p.h
    LIBRARIES
        WrapPNG::WrapPNG
)

#### Keys ignored in scope 40:.:image:image/image.pri:WIN32 AND MINGW:
# GCC_VERSION = "$${QMAKE_GCC_MAJOR_VERSION}.$${QMAKE_GCC_MINOR_VERSION}.$${QMAKE_GCC_PATCH_VERSION}"

qt_internal_extend_target(Gui CONDITION ((QT_FEATURE_png) AND (WIN32 AND MINGW)) AND (GCC_VERSION___equals___8.1.0)
    COMPILE_OPTIONS
        -fno-reorder-blocks-and-partition
)

if(NOT ANDROID)
    if(TEST_architecture_arch STREQUAL x86_64)
        qt_internal_extend_target(Gui
           SOURCES
                painting/qdrawhelper_sse2.cpp
        )
    else()
        qt_internal_add_simd_part(Gui SIMD sse2
            SOURCES
                painting/qdrawhelper_sse2.cpp
        )
    endif()

    qt_internal_add_simd_part(Gui SIMD ssse3
        SOURCES
            image/qimage_ssse3.cpp
            painting/qdrawhelper_ssse3.cpp
    )

    qt_internal_add_simd_part(Gui SIMD sse4_1
        SOURCES
            painting/qdrawhelper_sse4.cpp
            painting/qimagescale_sse4.cpp
    )

    qt_internal_add_simd_part(Gui SIMD neon
        SOURCES
            image/qimage_neon.cpp
            painting/qdrawhelper_neon.cpp painting/qdrawhelper_neon_p.h
            painting/qimagescale_neon.cpp
    )

    qt_internal_add_simd_part(Gui SIMD mips_dsp
        SOURCES
            painting/qdrawhelper_mips_dsp.cpp painting/qdrawhelper_mips_dsp_p.h
            painting/qdrawhelper_mips_dsp_asm.S
            painting/qt_mips_asm_dsp_p.h
    )

    qt_internal_add_simd_part(Gui SIMD mips_dspr2
        SOURCES
            image/qimage_mips_dspr2.cpp
            image/qimage_mips_dspr2_asm.S
            painting/qdrawhelper_mips_dspr2_asm.S
    )

    qt_internal_add_simd_part(Gui SIMD arch_haswell
        SOURCES
            painting/qdrawhelper_avx2.cpp
        EXCLUDE_OSX_ARCHITECTURES
            arm64
    )
endif()

qt_internal_extend_target(Gui CONDITION ANDROID
    SOURCES
        platform/android/qandroidnativeinterface.cpp
)

qt_internal_extend_target(Gui CONDITION ANDROID AND (TEST_architecture_arch STREQUAL arm64 OR TEST_architecture_arch STREQUAL arm) # special case
    SOURCES
        image/qimage_neon.cpp
        painting/qdrawhelper_neon.cpp painting/qdrawhelper_neon_p.h
        painting/qimagescale_neon.cpp
)

qt_internal_extend_target(Gui CONDITION ANDROID AND (TEST_architecture_arch STREQUAL i386 OR TEST_architecture_arch STREQUAL x86_64)
    SOURCES
        image/qimage_ssse3.cpp
        painting/qdrawhelper_sse2.cpp
        painting/qdrawhelper_ssse3.cpp
    DEFINES
        QT_COMPILER_SUPPORTS_SSE2 QT_COMPILER_SUPPORTS_SSE2
        QT_COMPILER_SUPPORTS_SSE3 QT_COMPILER_SUPPORTS_SSE3
        QT_COMPILER_SUPPORTS_SSSE3 QT_COMPILER_SUPPORTS_SSSE3
)

# special case begin
if (MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 8.1.0)
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86048
  set_source_files_properties(image/qpnghandler.cpp
    PROPERTIES COMPILE_OPTIONS -fno-reorder-blocks-and-partition
  )
endif()
# special case end

qt_internal_extend_target(Gui CONDITION QT_FEATURE_harfbuzz
    SOURCES
        text/qharfbuzzng.cpp text/qharfbuzzng_p.h
    LIBRARIES
        WrapHarfbuzz::WrapHarfbuzz
)

# special case begin
# Replicate what src/3rdparty/harfbuzz-ng/harfbuzz-ng.pro does, which is link CoreText
# when targeting uikit.

qt_internal_extend_target(Gui CONDITION QT_FEATURE_harfbuzz AND UIKIT
    LIBRARIES
        ${FWCoreText}
)
# special case end

qt_internal_extend_target(Gui CONDITION QT_FEATURE_textodfwriter
    SOURCES
        text/qtextodfwriter.cpp text/qtextodfwriter_p.h
        text/qzip.cpp
        text/qzipreader_p.h
        text/qzipwriter_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_textmarkdownreader
    SOURCES
        text/qtextmarkdownimporter.cpp text/qtextmarkdownimporter_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_system_textmarkdownreader AND QT_FEATURE_textmarkdownreader
    LIBRARIES
        WrapSystemMd4c::WrapSystemMd4c
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_textmarkdownreader AND NOT QT_FEATURE_system_textmarkdownreader
    SOURCES
        ../3rdparty/md4c/md4c.c ../3rdparty/md4c/md4c.h
    DEFINES
        MD4C_USE_UTF8
    INCLUDE_DIRECTORIES
        ../3rdparty/md4c
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_textmarkdownwriter
    SOURCES
        text/qtextmarkdownwriter.cpp text/qtextmarkdownwriter_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_cssparser
    SOURCES
        painting/qcssutil.cpp
        text/qcssparser.cpp text/qcssparser_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_freetype
    SOURCES
        text/freetype/qfontengine_ft.cpp text/freetype/qfontengine_ft_p.h
        text/freetype/qfreetypefontdatabase.cpp text/freetype/qfreetypefontdatabase_p.h
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_freetype AND UNIX AND NOT APPLE
    SOURCES
        text/unix/qgenericunixfontdatabase_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_fontconfig AND QT_FEATURE_freetype AND UNIX AND NOT APPLE
    SOURCES
        text/unix/qfontconfigdatabase.cpp text/unix/qfontconfigdatabase_p.h
        text/unix/qfontenginemultifontconfig.cpp text/unix/qfontenginemultifontconfig_p.h
    LIBRARIES
        Fontconfig::Fontconfig
)

qt_internal_extend_target(Gui CONDITION APPLE AND NOT MACOS
    LIBRARIES
        ${FWUIKit}
)

qt_internal_extend_target(Gui CONDITION APPLE AND QT_FEATURE_freetype
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_freetype AND WIN32
    SOURCES
        text/windows/qwindowsfontdatabase_ft.cpp text/windows/qwindowsfontdatabase_ft_p.h
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND WIN32
    SOURCES
        text/windows/qwindowsfontenginedirectwrite.cpp text/windows/qwindowsfontenginedirectwrite_p.h
    LIBRARIES
        d2d1
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND QT_FEATURE_directwrite3 AND WIN32
    SOURCES
        text/windows/qwindowsdirectwritefontdatabase.cpp text/windows/qwindowsdirectwritefontdatabase_p.h
    LIBRARIES
        dwrite # special case
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND WIN32 AND NOT QT_FEATURE_directwrite3
    LIBRARIES
        dwrite # special case
)

qt_internal_extend_target(Gui CONDITION MINGW AND WIN32
    LIBRARIES
        uuid
)

# Only enable the Pixman draw-helpers on platforms that support the GAS syntax of their asm files
# Note: These helpers are only used for 16-bit surfaces, so excluding them does not generally
# exclude neon-drawhelpers on these platforms.
if(UNIX AND NOT ANDROID AND NOT INTEGRITY AND NOT (TEST_architecture_arch STREQUAL "arm64") AND NOT UIKIT AND NOT QT_IS_MACOS_UNIVERSAL)
    qt_internal_extend_target(Gui DEFINES ENABLE_PIXMAN_DRAWHELPERS)
    qt_internal_add_simd_part(Gui SIMD neon
        SOURCES
            ../3rdparty/pixman/pixman-arm-neon-asm.S
            painting/qdrawhelper_neon_asm.S
    )
endif()

qt_internal_extend_target(Gui CONDITION ANDROID AND TEST_architecture_arch STREQUAL x86_64
    SOURCES
        painting/qdrawhelper_sse4.cpp
        painting/qimagescale_sse4.cpp
    DEFINES
        QT_COMPILER_SUPPORTS_SSE4_1
        QT_COMPILER_SUPPORTS_SSE4_2
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(Gui CONDITION NOT QT_FEATURE_system_zlib
    INCLUDE_DIRECTORIES
        ../3rdparty/zlib/src
)

qt_internal_extend_target(Gui CONDITION NOT QT_FEATURE_system_zlib AND NOT no_core_dep
    LIBRARIES
        Qt::Core
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_undocommand
    SOURCES
        util/qundostack.cpp util/qundostack.h util/qundostack_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_undogroup
    SOURCES
        util/qundogroup.cpp util/qundogroup.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_opengl AND QT_FEATURE_xlib AND NOT QT_FEATURE_opengles2
    SOURCES
        opengl/platform/unix/qglxconvenience.cpp opengl/platform/unix/qglxconvenience_p.h
    LIBRARIES
        X11::X11
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_egl AND QT_FEATURE_opengl
    SOURCES
        opengl/platform/egl/qeglpbuffer.cpp opengl/platform/egl/qeglpbuffer_p.h
        opengl/platform/egl/qeglplatformcontext.cpp opengl/platform/egl/qeglplatformcontext_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_egl AND QT_FEATURE_egl_x11
    SOURCES
        opengl/platform/egl/qxlibeglintegration.cpp opengl/platform/egl/qxlibeglintegration_p.h
    LIBRARIES
        X11::X11
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_egl AND NOT QT_FEATURE_egl_x11
    DEFINES
        QT_EGL_NO_X11
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_dlopen AND QT_FEATURE_egl
    LIBRARIES
        ${CMAKE_DL_LIBS}
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_standarditemmodel
    SOURCES
        itemmodels/qstandarditemmodel.cpp itemmodels/qstandarditemmodel.h itemmodels/qstandarditemmodel_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_filesystemmodel
    SOURCES
        itemmodels/qfileinfogatherer.cpp itemmodels/qfileinfogatherer_p.h
        itemmodels/qfilesystemmodel.cpp itemmodels/qfilesystemmodel.h itemmodels/qfilesystemmodel_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_vulkan
    SOURCES
        rhi/qrhivulkan.cpp rhi/qrhivulkan_p.h
        rhi/qrhivulkan_p_p.h
        vulkan/qbasicvulkanplatforminstance.cpp vulkan/qbasicvulkanplatforminstance_p.h
        vulkan/qplatformvulkaninstance.cpp vulkan/qplatformvulkaninstance.h
        vulkan/qvulkanfunctions.cpp
        vulkan/qvulkaninstance.cpp vulkan/qvulkaninstance.h
        vulkan/qvulkanwindow.cpp vulkan/qvulkanwindow.h vulkan/qvulkanwindow_p.h
)
if(QT_FEATURE_vulkan)
    qt_internal_add_target_include_dirs_and_optionally_propagate(
        Gui WrapVulkanHeaders::WrapVulkanHeaders)
endif()

#### Keys ignored in scope 111:.:vulkan:vulkan/vulkan.pri:QT_FEATURE_vkgen:
# special case begin
# We must always generate syncqt-injected header files,
# because we added a custom command earlier for those for framework builds.
set(vulkan_fun "qvulkanfunctions.h")
set(vulkan_fun_p "qvulkanfunctions_p.h")
set(vulkan_fun_outputs "vulkan/${vulkan_fun}" "vulkan/${vulkan_fun_p}")
set(vulkan_fun_command_content COMMAND "${CMAKE_COMMAND}" -E make_directory "vulkan")
if (QT_FEATURE_vulkan)
    set(vulkan_fun_p_cpp "qvulkanfunctions_p.cpp")
    list(APPEND vulkan_fun_outputs "vulkan/${vulkan_fun_p_cpp}")

    if(NOT "${QT_HOST_PATH}" STREQUAL "")
        set(host_executable_suffix "")
        if(CMAKE_HOST_WIN32)
            set(host_executable_suffix ".exe")
        endif()
        qt_path_join(qvkgen
            "${QT_HOST_PATH}"
            "${QT${PROJECT_VERSION_MAJOR}_HOST_INFO_LIBEXECDIR}"
            "qvkgen${host_executable_suffix}")
    else()
        set(qvkgen "${QT_CMAKE_EXPORT_NAMESPACE}::qvkgen")
    endif()

    list(APPEND vulkan_fun_command_content
        COMMAND "${qvkgen}"
            "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk.xml"
            "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/generated_header.txt"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/qvulkanfunctions"
        DEPENDS vulkan/vk.xml ${qvkgen}
        COMMENT "Generating vulkan data"
    )

    qt_internal_extend_target(Gui
        SOURCES
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${vulkan_fun}"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${vulkan_fun_p}"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${vulkan_fun_p_cpp}"
    )
else()
    foreach(file ${vulkan_fun_outputs})
        list(APPEND vulkan_fun_command_content
            COMMAND "${CMAKE_COMMAND}" -E touch "${file}")
    endforeach()
endif()
add_custom_command(
    OUTPUT ${vulkan_fun_outputs}
    ${vulkan_fun_command_content}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
# special case end

#### Keys ignored in scope 68:.:vulkan:vulkan/vulkan.pri:QT_FEATURE_vkgen:
# QMAKE_EXTRA_COMPILERS = "qvkgen_h" "qvkgen_ph" "qvkgen_pimpl"
# QMAKE_QVKGEN_INPUT = "vulkan/vk.xml"
# QMAKE_QVKGEN_LICENSE_HEADER = "$$QT_SOURCE_TREE/header.LGPL"
# qvkgen_h.commands = "$$QMAKE_QVKGEN" "${QMAKE_FILE_IN}" "$$shell_quote($$QMAKE_QVKGEN_LICENSE_HEADER)" "${QMAKE_FILE_OUT_PATH}/${QMAKE_FILE_OUT_BASE}"
# qvkgen_h.input = "QMAKE_QVKGEN_INPUT"
# qvkgen_h.output = "$$OUT_PWD/vulkan/qvulkanfunctions.h"
# qvkgen_ph.commands = "$$escape_expand(\\n)"
# qvkgen_ph.depends = "$$OUT_PWD/vulkan/qvulkanfunctions.h"
# qvkgen_ph.input = "QMAKE_QVKGEN_INPUT"
# qvkgen_ph.output = "$$OUT_PWD/vulkan/qvulkanfunctions_p.h"
# qvkgen_pimpl.commands = "$$escape_expand(\\n)"
# qvkgen_pimpl.depends = "$$OUT_PWD/vulkan/qvulkanfunctions_p.h"
# qvkgen_pimpl.input = "QMAKE_QVKGEN_INPUT"
# qvkgen_pimpl.output = "$$OUT_PWD/vulkan/qvulkanfunctions_p.cpp"

#### Keys ignored in scope 112:.:vulkan:vulkan/vulkan.pri:QT_FEATURE_vulkan:
# qvkgen_h.variable_out = "HEADERS"

#### Keys ignored in scope 113:.:vulkan:vulkan/vulkan.pri:else:
# qvkgen_h.CONFIG = "target_predeps" "no_link"

qt_internal_extend_target(Gui CONDITION WASM
    SOURCES
        platform/wasm/qwasmlocalfileaccess.cpp platform/wasm/qwasmlocalfileaccess_p.h
)

qt_internal_extend_target(Gui CONDITION UNIX
    SOURCES
        platform/unix/qgenericunixeventdispatcher.cpp platform/unix/qgenericunixeventdispatcher_p.h
        platform/unix/qunixeventdispatcher.cpp
        platform/unix/qunixeventdispatcher_qpa_p.h
        platform/unix/qunixnativeinterface.cpp
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_glib AND UNIX
    SOURCES
        platform/unix/qeventdispatcher_glib.cpp platform/unix/qeventdispatcher_glib_p.h
    LIBRARIES
        GLIB2::GLIB2
)

qt_internal_extend_target(Gui CONDITION UNIX AND (QT_FEATURE_xcb OR NOT UIKIT)
    SOURCES
        platform/unix/qgenericunixservices.cpp platform/unix/qgenericunixservices_p.h
)

qt_internal_extend_target(Gui CONDITION UNIX AND (QT_FEATURE_xcb)
    SOURCES
        platform/unix/qtx11extras.cpp platform/unix/qtx11extras_p.h
)

qt_internal_extend_target(Gui CONDITION TARGET Qt::DBus AND UNIX AND (QT_FEATURE_xcb OR NOT UIKIT)
    LIBRARIES
        Qt::DBus
)

qt_internal_extend_target(Gui CONDITION UNIX AND (QT_FEATURE_xcb OR NOT MACOS) AND (QT_FEATURE_xcb OR NOT UIKIT)
    SOURCES
        platform/unix/qgenericunixthemes.cpp platform/unix/qgenericunixthemes_p.h
)

qt_internal_extend_target(Gui CONDITION TARGET Qt::DBus AND UNIX AND (QT_FEATURE_xcb OR NOT MACOS) AND (QT_FEATURE_xcb OR NOT UIKIT)
    SOURCES
        platform/unix/dbusmenu/qdbusmenuadaptor.cpp platform/unix/dbusmenu/qdbusmenuadaptor_p.h
        platform/unix/dbusmenu/qdbusmenubar.cpp platform/unix/dbusmenu/qdbusmenubar_p.h
        platform/unix/dbusmenu/qdbusmenuconnection.cpp platform/unix/dbusmenu/qdbusmenuconnection_p.h
        platform/unix/dbusmenu/qdbusmenuregistrarproxy.cpp platform/unix/dbusmenu/qdbusmenuregistrarproxy_p.h
        platform/unix/dbusmenu/qdbusmenutypes.cpp platform/unix/dbusmenu/qdbusmenutypes_p.h
        platform/unix/dbusmenu/qdbusplatformmenu.cpp platform/unix/dbusmenu/qdbusplatformmenu_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_systemtrayicon AND TARGET Qt::DBus AND UNIX AND (QT_FEATURE_xcb OR NOT MACOS) AND (QT_FEATURE_xcb OR NOT UIKIT)
    SOURCES
        platform/unix/dbustray/qdbustrayicon.cpp platform/unix/dbustray/qdbustrayicon_p.h
        platform/unix/dbustray/qdbustraytypes.cpp platform/unix/dbustray/qdbustraytypes_p.h
        platform/unix/dbustray/qstatusnotifieritemadaptor.cpp platform/unix/dbustray/qstatusnotifieritemadaptor_p.h
        platform/unix/dbustray/qxdgnotificationproxy.cpp platform/unix/dbustray/qxdgnotificationproxy_p.h
)

qt_internal_extend_target(Gui CONDITION QT_FEATURE_xkbcommon AND UNIX
    SOURCES
        platform/unix/qxkbcommon.cpp platform/unix/qxkbcommon_p.h
        platform/unix/qxkbcommon_3rdparty.cpp
    LIBRARIES
        XKB::XKB
# special case begin
    PRIVATE_MODULE_INTERFACE
        XKB::XKB
# special case end
)

qt_internal_extend_target(Gui CONDITION IOS OR MACOS
    SOURCES
        rhi/qrhimetal.mm rhi/qrhimetal_p.h
        rhi/qrhimetal_p_p.h
    PUBLIC_LIBRARIES
        ${FWMetal}
)

qt_internal_extend_target(Gui # special case CONDITION NOT GCC OR NOT QT_COMPILER_VERSION_MAJOR STREQUAL 5 # source subtraction gone wrong
    SOURCES
        painting/qdrawhelper.cpp
    NO_PCH_SOURCES
        "painting/qdrawhelper.cpp"
)

qt_internal_extend_target(Gui CONDITION (QT_FEATURE_eglfs OR QT_FEATURE_xcb)
    SOURCES
        util/qedidparser.cpp util/qedidparser_p.h
        util/qedidvendortable_p.h
)


qt_internal_create_tracepoints(Gui qtgui.tracepoints)
qt_internal_add_docs(Gui
    doc/qtgui.qdocconf
)

# special case begin
qt_internal_add_optimize_full_flags()
# special case end
