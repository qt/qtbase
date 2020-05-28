# Generated from gui.pro.

set(OpenGL_GL_PREFERENCE GLVND) # special case:

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
        set(_default_platform "integrityfb")
    elseif(HAIKU)
        set(_default_platform "haiku")
    elseif(WASM)
        set(_default_platform "wasm")
    else()
        set(_default_platform "xcb")
    endif()

    set(QT_QPA_DEFAULT_PLATFORM "${_default_platform}" CACHE STRING "QPA default platform")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties(../3rdparty/md4c/md4c.c PROPERTIES COMPILE_FLAGS "-Wno-error=unused-parameter -Wno-error=sign-compare -Wno-error=missing-field-initializers")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set_source_files_properties(../3rdparty/md4c/md4c.c PROPERTIES COMPILE_FLAGS "-Wno-error=unused-parameter -Wno-error=sign-compare -Wno-error=missing-field-initializers -Wno-error=missing-braces")
endif()

unset(qmake_module_config)
if(QT_FEATURE_opengl)
    list(APPEND qmake_module_config opengl)
endif()

# special case end

qt_add_module(Gui
    GENERATE_METATYPES
    PLUGIN_TYPES accessiblebridge platforms platforms/darwin xcbglintegrations platformthemes platforminputcontexts generic iconengines imageformats egldeviceintegrations
    FEATURE_DEPENDENCIES  # special case:
        Qt::Network # special case:
    QMAKE_MODULE_CONFIG "${qmake_module_config}" # special case
    SOURCES
        image/qabstractfileiconengine.cpp image/qabstractfileiconengine_p.h
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
        painting/qpolygonclipper_p.h
        painting/qrangecollection.cpp painting/qrangecollection.h painting/qrangecollection_p.h
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
set_source_files_properties("painting/../../3rdparty/icc/sRGB2014.icc"
    PROPERTIES QT_RESOURCE_ALIAS "sRGB2014.icc"
)
set(qpdf_resource_files
    "../../3rdparty/icc/sRGB2014.icc"
    "qpdfa_metadata.xml"
)

qt_add_resource(Gui "qpdf"
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
# Replace arch_haswell with avx2 feature. That is what is used in the code requireing
# this file, too!
qt_add_simd_part(Gui SIMD avx2
    SOURCES
        painting/qdrawhelper_avx2.cpp
)
# special case end

# special case begin
qt_extend_target(Gui CONDITION QT_FEATURE_standarditemmodel
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
# available until the qt_add_module call.
if(QT_FEATURE_opengl)
    if(QT_FEATURE_opengles2)
        find_package(GLESv2)
        target_link_libraries(Gui PUBLIC GLESv2::GLESv2)
    elseif(NOT QT_FEATURE_opengl_dynamic)
        find_package(OpenGL)
        target_link_libraries(Gui PUBLIC OpenGL::GL)
    endif()
endif()
# special case end

qt_extend_target(Gui CONDITION QT_FEATURE_opengl
    SOURCES
        kernel/qopenglcontext.cpp kernel/qopenglcontext.h kernel/qopenglcontext_p.h
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

qt_extend_target(Gui CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
    PUBLIC_LIBRARIES
        ${FWAppKit}
)

qt_extend_target(Gui CONDITION APPLE
    SOURCES
        image/qimage_darwin.mm
        painting/qcoregraphics.mm painting/qcoregraphics_p.h
        painting/qrasterbackingstore.cpp painting/qrasterbackingstore_p.h
        platform/darwin/qmacmime.mm platform/darwin/qmacmime_p.h
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

qt_extend_target(Gui CONDITION QT_FEATURE_animation
    SOURCES
        animation/qguivariantanimation.cpp
)

qt_extend_target(Gui CONDITION WIN32
    SOURCES
        image/qpixmap_win.cpp
        kernel/qwindowdefs_win.h
        platform/windows/qwindowsguieventdispatcher.cpp platform/windows/qwindowsguieventdispatcher_p.h
        rhi/qrhid3d11.cpp rhi/qrhid3d11_p.h
        rhi/qrhid3d11_p_p.h
    PUBLIC_LIBRARIES
        d3d11
        dxgi
        dxguid
)

#### Keys ignored in scope 7:.:.:gui.pro:WIN32:
# CMAKE_WINDOWS_BUILD = "True"

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

qt_extend_target(Gui CONDITION QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessible.cpp accessible/qaccessible.h
        accessible/qaccessiblebridge.cpp accessible/qaccessiblebridge.h
        accessible/qaccessiblebridgeutils.cpp accessible/qaccessiblebridgeutils_p.h
        accessible/qaccessiblecache.cpp accessible/qaccessiblecache_p.h
        accessible/qaccessibleobject.cpp accessible/qaccessibleobject.h
        accessible/qaccessibleplugin.cpp accessible/qaccessibleplugin.h
        accessible/qplatformaccessibility.cpp accessible/qplatformaccessibility.h
)

qt_extend_target(Gui CONDITION APPLE AND QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessiblecache_mac.mm
    LIBRARIES
        ${FWFoundation}
)

qt_extend_target(Gui CONDITION QT_FEATURE_accessibility AND WIN32
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

qt_extend_target(Gui CONDITION QT_FEATURE_action
    SOURCES
        kernel/qaction.cpp kernel/qaction.h kernel/qaction_p.h
        kernel/qactiongroup.cpp kernel/qactiongroup.h kernel/qactiongroup_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_draganddrop
    SOURCES
        kernel/qdnd.cpp kernel/qdnd_p.h
        kernel/qdrag.cpp kernel/qdrag.h
        kernel/qplatformdrag.cpp kernel/qplatformdrag.h
        kernel/qshapedpixmapdndwindow.cpp kernel/qshapedpixmapdndwindow_p.h
        kernel/qsimpledrag.cpp kernel/qsimpledrag_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_shortcut
    SOURCES
        kernel/qkeysequence.cpp kernel/qkeysequence.h kernel/qkeysequence_p.h
        kernel/qshortcut.cpp kernel/qshortcut.h kernel/qshortcut_p.h
        kernel/qshortcutmap.cpp kernel/qshortcutmap_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_movie
    SOURCES
        image/qmovie.cpp image/qmovie.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_png
    SOURCES
        image/qpnghandler.cpp image/qpnghandler_p.h
    LIBRARIES
        WrapPNG::WrapPNG
)

#### Keys ignored in scope 38:.:image:image/image.pri:WIN32 AND MINGW:
# GCC_VERSION = "$${QMAKE_GCC_MAJOR_VERSION}.$${QMAKE_GCC_MINOR_VERSION}.$${QMAKE_GCC_PATCH_VERSION}"

qt_extend_target(Gui CONDITION ((QT_FEATURE_png) AND (WIN32 AND MINGW)) AND (GCC_VERSION___equals___8.1.0)
    COMPILE_OPTIONS
        -fno-reorder-blocks-and-partition
)

if(NOT ANDROID)
    qt_add_simd_part(Gui SIMD sse2
        SOURCES
            painting/qdrawhelper_sse2.cpp
    )

    qt_add_simd_part(Gui SIMD ssse3
        SOURCES
            image/qimage_ssse3.cpp
            painting/qdrawhelper_ssse3.cpp
    )

    qt_add_simd_part(Gui SIMD sse4_1
        SOURCES
            painting/qdrawhelper_sse4.cpp
            painting/qimagescale_sse4.cpp
    )

    qt_add_simd_part(Gui SIMD neon
        SOURCES
            image/qimage_neon.cpp
            painting/qdrawhelper_neon.cpp painting/qdrawhelper_neon_p.h
            painting/qimagescale_neon.cpp
    )

    qt_add_simd_part(Gui SIMD mips_dsp
        SOURCES
            painting/qdrawhelper_mips_dsp.cpp painting/qdrawhelper_mips_dsp_p.h
            painting/qdrawhelper_mips_dsp_asm.S
            painting/qt_mips_asm_dsp_p.h
    )

    qt_add_simd_part(Gui SIMD mips_dspr2
        SOURCES
            image/qimage_mips_dspr2.cpp
            image/qimage_mips_dspr2_asm.S
            painting/qdrawhelper_mips_dspr2_asm.S
    )

    qt_add_simd_part(Gui SIMD arch_haswell
        SOURCES
            painting/qdrawhelper_avx2.cpp
    )
endif()

qt_extend_target(Gui CONDITION ANDROID AND (TEST_architecture_arch STREQUAL arm64 OR TEST_architecture_arch STREQUAL arm) # special case
    SOURCES
        image/qimage_neon.cpp
        painting/qdrawhelper_neon.cpp painting/qdrawhelper_neon_p.h
        painting/qimagescale_neon.cpp
)

qt_extend_target(Gui CONDITION ANDROID AND (TEST_architecture_arch STREQUAL i386 OR TEST_architecture_arch STREQUAL x86_64)
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

qt_extend_target(Gui CONDITION QT_FEATURE_harfbuzz
    SOURCES
        text/qharfbuzzng.cpp text/qharfbuzzng_p.h
    LIBRARIES
        WrapHarfbuzz::WrapHarfbuzz
)

# special case begin
# Replicate what src/3rdparty/harfbuzz-ng/harfbuzz-ng.pro does, which is link CoreText
# when targeting uikit.

qt_extend_target(Gui CONDITION QT_FEATURE_harfbuzz AND UIKIT
    LIBRARIES
        ${FWCoreText}
)
# special case end

qt_extend_target(Gui CONDITION QT_FEATURE_textodfwriter
    SOURCES
        text/qtextodfwriter.cpp text/qtextodfwriter_p.h
        text/qzip.cpp
        text/qzipreader_p.h
        text/qzipwriter_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_textmarkdownreader
    SOURCES
        text/qtextmarkdownimporter.cpp text/qtextmarkdownimporter_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_system_textmarkdownreader AND QT_FEATURE_textmarkdownreader
    LIBRARIES
        libmd4c
)

qt_extend_target(Gui CONDITION QT_FEATURE_textmarkdownreader AND NOT QT_FEATURE_system_textmarkdownreader
    SOURCES
        ../3rdparty/md4c/md4c.c ../3rdparty/md4c/md4c.h
    DEFINES
        MD4C_USE_UTF8
    INCLUDE_DIRECTORIES
        ../3rdparty/md4c
)

qt_extend_target(Gui CONDITION QT_FEATURE_textmarkdownwriter
    SOURCES
        text/qtextmarkdownwriter.cpp text/qtextmarkdownwriter_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_cssparser
    SOURCES
        painting/qcssutil.cpp
        text/qcssparser.cpp text/qcssparser_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_freetype
    SOURCES
        text/freetype/qfontengine_ft.cpp text/freetype/qfontengine_ft_p.h
        text/freetype/qfreetypefontdatabase.cpp text/freetype/qfreetypefontdatabase_p.h
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_extend_target(Gui CONDITION APPLE AND NOT MACOS
    LIBRARIES
        ${FWUIKit}
)

qt_extend_target(Gui CONDITION APPLE AND QT_FEATURE_freetype
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_extend_target(Gui CONDITION UNIX AND NOT ANDROID AND NOT INTEGRITY AND NOT (TEST_architecture_arch STREQUAL "arm64") AND NOT UIKIT
    DEFINES
        ENABLE_PIXMAN_DRAWHELPERS
)

if(UNIX AND NOT ANDROID AND NOT INTEGRITY AND NOT (TEST_architecture_arch STREQUAL "arm64") AND NOT UIKIT)
    qt_add_simd_part(Gui SIMD neon
        SOURCES
            ../3rdparty/pixman/pixman-arm-neon-asm.S
            painting/qdrawhelper_neon_asm.S
    )
endif()

qt_extend_target(Gui CONDITION ANDROID AND TEST_architecture_arch STREQUAL x86_64
    SOURCES
        painting/qdrawhelper_sse4.cpp
        painting/qimagescale_sse4.cpp
    DEFINES
        QT_COMPILER_SUPPORTS_SSE4_1
        QT_COMPILER_SUPPORTS_SSE4_2
)

qt_extend_target(Gui CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        ZLIB::ZLIB
)

qt_extend_target(Gui CONDITION NOT QT_FEATURE_system_zlib
    INCLUDE_DIRECTORIES
        ../3rdparty/zlib/src
)

qt_extend_target(Gui CONDITION NOT QT_FEATURE_system_zlib AND NOT no_core_dep
    LIBRARIES
        Qt::Core
)

qt_extend_target(Gui CONDITION QT_FEATURE_regularexpression
    SOURCES
        util/qshadergenerator.cpp util/qshadergenerator_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_undocommand
    SOURCES
        util/qundostack.cpp util/qundostack.h util/qundostack_p.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_undogroup
    SOURCES
        util/qundogroup.cpp util/qundogroup.h
)

qt_extend_target(Gui CONDITION QT_FEATURE_vulkan
    SOURCES
        rhi/qrhivulkan.cpp rhi/qrhivulkan_p.h
        rhi/qrhivulkan_p_p.h
        vulkan/qbasicvulkanplatforminstance.cpp vulkan/qbasicvulkanplatforminstance_p.h
        vulkan/qplatformvulkaninstance.cpp vulkan/qplatformvulkaninstance.h
        vulkan/qvulkanfunctions.cpp
        vulkan/qvulkaninstance.cpp vulkan/qvulkaninstance.h
        vulkan/qvulkanwindow.cpp vulkan/qvulkanwindow.h vulkan/qvulkanwindow_p.h
    PUBLIC_LIBRARIES
        Vulkan::Vulkan_nolink
)

#### Keys ignored in scope 89:.:vulkan:vulkan/vulkan.pri:QT_FEATURE_vkgen:
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

    list(APPEND vulkan_fun_command_content
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qvkgen
            "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk.xml"
            "${PROJECT_SOURCE_DIR}/header.LGPL"
            "${CMAKE_CURRENT_BINARY_DIR}/vulkan/qvulkanfunctions"
        DEPENDS vulkan/vk.xml
        COMMENT "Generating vulkan data"
    )

    extend_target(Gui
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

#### Keys ignored in scope 90:.:vulkan:vulkan/vulkan.pri:QT_FEATURE_vulkan:
# qvkgen_h.variable_out = "HEADERS"

#### Keys ignored in scope 91:.:vulkan:vulkan/vulkan.pri:else:
# qvkgen_h.CONFIG = "target_predeps" "no_link"

qt_extend_target(Gui CONDITION WASM
    SOURCES
        platform/wasm/qwasmlocalfileaccess.cpp platform/wasm/qwasmlocalfileaccess_p.h
)

qt_extend_target(Gui CONDITION IOS OR MACOS
    SOURCES
        rhi/qrhimetal.mm rhi/qrhimetal_p.h
        rhi/qrhimetal_p_p.h
    PUBLIC_LIBRARIES
        ${FWMetal}
)

qt_extend_target(Gui # special case CONDITION NOT GCC OR NOT QT_COMPILER_VERSION_MAJOR STREQUAL 5 # source subtraction gone wrong
    SOURCES
        painting/qdrawhelper.cpp
    NO_PCH_SOURCES
        "painting/qdrawhelper.cpp"
)


qt_create_tracepoints(Gui qtgui.tracepoints)
qt_add_docs(Gui
    doc/qtgui.qdocconf
)

